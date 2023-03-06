#if ProtobufReflectionSupport
using System;
using System.Text;
using System.IO;
using Google.Protobuf;
using System.Collections.Generic;

namespace Continental
{
  namespace eCAL
  {
    namespace Core
    {

      internal static class ProtobufDynamicDecoder
      {
        public static Google.Protobuf.MessageParser GetMessageParserFromDescriptor(byte[] descriptor_, string typename_)
        {
          var pset_ = new Google.Protobuf.Reflection.FileDescriptorSet();
          var stream_ = new Google.Protobuf.CodedInputStream(descriptor_);
          pset_.MergeFrom(stream_);

          return GetMessageParserFromDescriptorSet(pset_, typename_);
        }

        public static Google.Protobuf.MessageParser GetMessageParserFromDescriptorSet(Google.Protobuf.Reflection.FileDescriptorSet descriptor_set_, string typename_)
        {
          var descriptor_list = new List<ByteString>(); 
          foreach (var file in descriptor_set_.File)
          {
            descriptor_list.Add(file.ToByteString());
          }
          var all_descriptors = Google.Protobuf.Reflection.FileDescriptor.BuildFromByteStrings(descriptor_list);
          var registry = Google.Protobuf.Reflection.TypeRegistry.FromFiles(all_descriptors);
          var message_descriptor = registry.Find(typename_);

          return message_descriptor.Parser;
          //file_descriptor_.FindTypeByName<Google.Protobuf.Reflection.MessageDescriptor>(typename_);
          //Google.Protobuf.Reflection.MessageF
        }
      }

      /**
       * @brief eCAL class to subscribe to protobuf Data.
      **/
      public class ProtobufDynamicSubscriber
      {
        private Subscriber binarySubscriber;
        private ReceiverCallback callback;
        private Google.Protobuf.MessageParser messageParser;
        private string topicName;

        /**
        * @brief Data which is received from a callback
        **/
        public class ReceiveCallbackData
        {
          public Google.Protobuf.IMessage data; /*!< Message             */
          public long id;                       /*!< Message id          */
          public long time;                     /*!< Message time stamp  */
          public long clock;                    /*!< Message write clock */

        };

        private ReceiveCallbackData receivedData;

        /**
        * @brief Signature for a data callback.
        **/
        public delegate void ReceiverCallback(String topic, ReceiveCallbackData data);
        private ReceiverCallback delMethods;

        /**
        * @brief Constructor for a Protobuf Subscriber
        * 
        * @param topicName Topic name on which the subscriber subscribes to data.
        **/
        public ProtobufDynamicSubscriber(string topicName)
        {
          binarySubscriber = new Subscriber(topicName, "", new byte[0]);
          receivedData = new ReceiveCallbackData();
          this.topicName = topicName;
         }

        /**
        * @brief Add a callback function to this subscriber
        * 
        * @param callbackFunction function which will be called when new data is available.
        **/
        public void AddReceiveCallback(ReceiverCallback callbackFunction)
        {
          this.callback = callbackFunction;
          delMethods += callbackFunction;
          binarySubscriber.AddReceiveCallback(callBack);
        }

        /**
        * @brief Remove a callback function from this subscriber
        * 
        * @param callbackFunction function to be removed from the callback list.
        **/
        public void RemReceiveCallback(ReceiverCallback del)
        {
          binarySubscriber.RemReceiveCallback(callBack);
          delMethods -= del;
          this.callback = null;
        }

        private void callBack(String topic, Core.Subscriber.ReceiveCallbackData data)
        {
          System.Console.WriteLine("In Callback");

          if (messageParser == null)
          {
            BuildMessageParser();
          }

          if (messageParser != null)
          {
            //String to Stream
            byte[] messageBytes = Encoding.Default.GetBytes(data.data);
            MemoryStream msgStream = new MemoryStream(messageBytes);

            receivedData.data = messageParser.ParseFrom(msgStream);
            receivedData.id = data.id;
            receivedData.clock = data.clock;
            receivedData.time = data.time;
            //Execute passed methods
            delMethods(topic, receivedData);
          }
          else
          {
            System.Console.WriteLine("MessageParse not initialized!");
            //eCAL.Core.Logger.Log()
          }
        }

        /**
        * @brief Unregister this subscriber, no more data will be received
        **/
        public void finalized()
        {
          delMethods -= callback;
          binarySubscriber.RemReceiveCallback(callBack);
          binarySubscriber.Dispose();
          binarySubscriber.Destroy();
        }

        /**
        * @brief Receive data.
        * 
        * @param receiveTimeout Timeout after which the function returns, even if no data was received.
        *        If -1, it will only return after data was received.
        *        
        * @return  The received data, can be null if no data was received.
        **/
        public ReceiveCallbackData Receive(int receiveTimeout)
        {
          var receivedBinaryData = binarySubscriber.Receive(receiveTimeout);
          if (receivedBinaryData != null)
          {
            if (messageParser == null)
            {
              BuildMessageParser();
            }

            if (messageParser != null)
            {
              byte[] msgBytes = Encoding.Default.GetBytes(receivedBinaryData.data);
              var msgStream = new MemoryStream(msgBytes);

              receivedData.data = messageParser.ParseFrom(msgStream);
              receivedData.clock = receivedBinaryData.clock;
              receivedData.id = receivedBinaryData.id;
              receivedData.time = receivedBinaryData.time;
              return receivedData;
            }
            else
            {
              return null;
            }
          }
          else
          {
            return null;
          }
        }

        private void BuildMessageParser()
        {
          var descriptor = eCAL.Core.Util.GetTopicDescription(topicName);
          var typename = eCAL.Core.Util.GetTopicTypeName(topicName);
          char[] separator = { ':' };
          var split = typename.Split(separator);
          messageParser = ProtobufDynamicDecoder.GetMessageParserFromDescriptor(descriptor, split[1]);
        }
      }
    }
  }
}
#endif
