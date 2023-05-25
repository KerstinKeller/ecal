#include <ecal/measurement/mcap/writer.h>
#include <optional>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>

#define MCAP_IMPLEMENTATION
#include <mcap/writer.hpp>

namespace {
  const std::string writer_id{ "ecal_mcap" };
}

class MinimalWriterInterface
{
public:
  MinimalWriterInterface() = default;
  virtual ~MinimalWriterInterface() = default;
  MinimalWriterInterface(const MinimalWriterInterface& other) = default;
  MinimalWriterInterface& operator=(const MinimalWriterInterface& other) = default;
  MinimalWriterInterface(MinimalWriterInterface&&) = default;
  MinimalWriterInterface& operator=(MinimalWriterInterface&&) = default;

  virtual void SetChannelMetaInformation(const std::string& channel_name, const std::string& channel_type, const std::string& channel_descriptor) = 0;
  virtual bool AddEntryToFile(const void* data, const unsigned long long& size, const long long& snd_timestamp, const long long& rcv_timestamp, const std::string& channel_name, long long id, long long clock) = 0;
};

using WriterInterfaceCreator = std::function<std::unique_ptr<MinimalWriterInterface>(const std::string& path)>;

// This class is not thread safe!
class MinimalMcapWriter : public MinimalWriterInterface
{
public:
  MinimalMcapWriter(const std::string& path, const ::mcap::McapWriterOptions& writer_options) 
    : writer_options(writer_options)
    , writer(std::make_unique< ::mcap::McapWriter>())
  {
    writer->open(path, writer_options);
  }

  ~MinimalMcapWriter()
  {
    writer->close();
  }

  bool AddEntryToFile(const void* data, const unsigned long long& size, const long long& snd_timestamp, const long long& rcv_timestamp, const std::string& channel_name, long long id, long long clock)
  {
    auto it = channel_id_mapping.find(channel_name);
    if (it != channel_id_mapping.end())
    {
      ::mcap::Message msg;
      msg.logTime = rcv_timestamp * 1000;
      msg.publishTime = snd_timestamp * 1000;
      msg.channelId = it->second;
      msg.data = reinterpret_cast<const std::byte*>(data);
      msg.dataSize = size;
      auto status =  writer->write(msg);
      return status.ok();
    }
    else
    {
      return false;
    }
  }

  void SetChannelMetaInformation(const std::string& channel_name, const std::string& channel_type, const std::string& channel_descriptor)
  {
    // insert info only once!
    if (channel_id_mapping.find(channel_name) == channel_id_mapping.end())
    {
      // split type at ":" -> this is quite a bit incorrect, we need to review
      auto topic_type = channel_type.substr(channel_type.find_first_of(':') + 1, channel_type.size());
      auto schema = mcap::Schema(topic_type, "protobuf", channel_descriptor);
      writer->addSchema(schema);
      auto channel = mcap::Channel(channel_name, "protobuf", schema.id);
      writer->addChannel(channel);
      channel_id_mapping[channel_name] = channel.id;
    }
  }

private:
    ::mcap::McapWriterOptions writer_options;
    std::unique_ptr<::mcap::McapWriter> writer;
    std::unordered_map<std::string, ::mcap::ChannelId> channel_id_mapping;
};

class PerChannelWriter : public MinimalWriterInterface
{
public:
  PerChannelWriter(const std::string& path_, WriterInterfaceCreator writer_creator_)
    : path(path_)
    , writer_creator(writer_creator_)
  {}

  void SetChannelMetaInformation(const std::string& channel_name, const std::string& channel_type, const std::string& channel_descriptor)
  {
    auto it = FindOrCreateWriter(channel_name);
    return it->second->SetChannelMetaInformation(channel_name, channel_type, channel_descriptor);
  }

  bool AddEntryToFile(const void* data, const unsigned long long& size, const long long& snd_timestamp, const long long& rcv_timestamp, const std::string& channel_name, long long id, long long clock)
  {
    auto it = FindOrCreateWriter(channel_name);
    it->second->AddEntryToFile(data, size, snd_timestamp, rcv_timestamp, channel_name, id, clock);
    return true;  
  }

private:
  std::map<std::string, std::unique_ptr<MinimalWriterInterface>>::iterator FindOrCreateWriter(const std::string& channel_name)
  {
    auto it = channel_writer_map.find(channel_name);
    if (it != channel_writer_map.end())
    {
      it = CreateNewChannelWriter(channel_name);
    }
    return it;
  }

  std::map<std::string, std::unique_ptr<MinimalWriterInterface>>::iterator CreateNewChannelWriter(const std::string& channel_name)
  {
    auto complete_path = path + "/" + channel_name;
    auto inserted = channel_writer_map.insert({ channel_name, writer_creator(complete_path) });
    return inserted.first;
  }

  std::string path;
  WriterInterfaceCreator writer_creator;
  std::map<std::string, std::unique_ptr<MinimalWriterInterface>> channel_writer_map;
};

class SizeSplitWriter : public MinimalWriterInterface
{
public:
  SizeSplitWriter(const std::string& path_, size_t split_size_, WriterInterfaceCreator writer_creator_)
    : path(path_)
    , writer_number(0)
    , bytes_written(0)
    , split_size(split_size_)
    , currently_open_writer(writer_creator_(path_))
    , writer_creator(writer_creator_)
  {}

  void SetChannelMetaInformation(const std::string& channel_name, const std::string& channel_type, const std::string& channel_descriptor)
  {
    accumulated_meta_information.insert({ channel_name, channel_type, channel_descriptor });
    currently_open_writer->SetChannelMetaInformation(channel_name, channel_type, channel_descriptor);
  }

  bool AddEntryToFile(const void* data, const unsigned long long& size, const long long& snd_timestamp, const long long& rcv_timestamp, const std::string& channel_name, long long id, long long clock)
  {
    if (NeedToStartNewWriter(size))
    {
      StartNewWriter();
    }
    currently_open_writer->AddEntryToFile(data, size, snd_timestamp, rcv_timestamp, channel_name, id, clock);
    bytes_written += size;
  }

private:
  bool NeedToStartNewWriter(unsigned long long size)
  {
    return (bytes_written + size > split_size);
  }

  void StartNewWriter()
  {
    bytes_written = 0;
    ++writer_number;
    auto path_name = CreatePathName();
    currently_open_writer = writer_creator(path_name);
    RegisterExistingMetaInfo();
  }

  std::string CreatePathName()
  {
    return path + "_" + std::to_string(writer_number);
  }

  void RegisterExistingMetaInfo()
  {
    // Register all meta information with the new writer
    for (const auto& info : accumulated_meta_information)
    {
      currently_open_writer->SetChannelMetaInformation(std::get<0>(info), std::get<1>(info), std::get<2>(info));
    }
  }

  std::string path;
  unsigned int writer_number;
  
  size_t bytes_written;
  size_t split_size;

  std::unique_ptr<MinimalWriterInterface> currently_open_writer;

  WriterInterfaceCreator writer_creator;

  std::set<std::tuple<std::string, std::string, std::string>> accumulated_meta_information;
};

using SizeSplittingStrategy = std::optional<uint32_t>;

  enum class ChannelSplittingStrategy
  {
    NoSplitting = 0,
    OneChannelPerFile = 1
  };

  struct WriterConfigurationOptions
  {
    SizeSplittingStrategy size_splitting_strategy = 512;
    ChannelSplittingStrategy channel_splitting_stategy = ChannelSplittingStrategy::NoSplitting;
  };


  class WriterImplementation
  {
  public:
    std::unique_ptr<MinimalWriterInterface> writer;
    WriterConfigurationOptions options;
    std::string base_filename;
  };



eCAL::measurement::mcap::Writer::Writer() :
  implementation(std::make_unique<WriterImplementation>())
{}

  /**
   * @brief Constructor
  **/
eCAL::measurement::mcap::Writer::Writer(const std::string& path)
  : implementation(std::make_unique<WriterImplementation>())
{
  Open(path);
}

bool eCAL::measurement::mcap::Writer::Open(const std::string& path) { 
  ::mcap::McapWriterOptions options(writer_id);
  options.enableDataCRC = false;
  options.compression = ::mcap::Compression::None;
  std::string meas_path = path + implementation->base_filename + "meas.mcap";
  implementation->writer = std::make_unique<MinimalMcapWriter>(meas_path, options);
  return true;
}

bool eCAL::measurement::mcap::Writer::Close(){ 
  implementation.reset();
  return true; 
}

bool eCAL::measurement::mcap::Writer::IsOk() const { return true; }

size_t eCAL::measurement::mcap::Writer::GetMaxSizePerFile() const {
  return implementation->options.size_splitting_strategy.value_or(std::numeric_limits<size_t>::max());
}

void eCAL::measurement::mcap::Writer::SetMaxSizePerFile(size_t size) {
  implementation->options.size_splitting_strategy = size;
}

bool eCAL::measurement::mcap::Writer::IsOneFilePerChannelEnabled() const {
  return implementation->options.channel_splitting_stategy == ChannelSplittingStrategy::OneChannelPerFile;
}

void eCAL::measurement::mcap::Writer::SetOneFilePerChannelEnabled(bool enabled) {
  if (enabled)
    implementation->options.channel_splitting_stategy = ChannelSplittingStrategy::OneChannelPerFile;
  else
    implementation->options.channel_splitting_stategy = ChannelSplittingStrategy::NoSplitting;
}

// we need this info atomically. E.g. in Mcap this is a schma and it has both information
void eCAL::measurement::mcap::Writer::SetChannelMetaInformation(const std::string& channel_name, const std::string& channel_type, const std::string& channel_descriptor) {
  if (implementation->writer != nullptr)
  {
    implementation->writer->SetChannelMetaInformation(channel_name, channel_type, channel_descriptor);
  }
}

// This is something which a generic class should handle for both mcap and hdft
void eCAL::measurement::mcap::Writer::SetFileBaseName(const std::string& base_name) {
  implementation->base_filename = base_name;
}

bool eCAL::measurement::mcap::Writer::AddEntryToFile(const void* data, const unsigned long long& size, const long long& snd_timestamp, const long long& rcv_timestamp, const std::string& channel_name, long long id, long long clock) { 
  if (implementation->writer != nullptr)
  {
    return implementation->writer->AddEntryToFile(data, size, snd_timestamp, rcv_timestamp, channel_name, id, clock);
  }
  else
  {
    return false;
  }
}

