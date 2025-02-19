#include <QAbstractItemModel>
#include <QVariant>
#include <QString>
#include <QVector>
#include <QSet>
#include <QTreeView>
#include <QDebug>
#include <algorithm>
#include <memory>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>

constexpr int BATCH_SIZE = 10;

//------------------------------------------------------------------------------
// A node in our lazy protobuf tree.
class LazyProtobufNode {
public:
  enum class NodeType {
    Message,       // A protobuf message (has fields)
    RepeatedField, // A container for a repeated field
    Value          // A leaf value (primitive)
  };

  NodeType type;
  QString name;
  QString path; // Unique identifier (e.g. "Root/field/subField")
  LazyProtobufNode* parent = nullptr;

  // For Message nodes: a shared_ptr to the protobuf message.
  std::shared_ptr<const google::protobuf::Message> message;

  // For RepeatedField nodes: the parent message and field descriptor.
  std::shared_ptr<const google::protobuf::Message> parentMessage;
  const google::protobuf::FieldDescriptor* fieldDesc = nullptr;

  // For repeated field elements:
  int repeatedIndex = -1;

  // For Value nodes:
  QVariant value;

  // Lazy loading state:
  bool childrenFullyLoaded = false;
  int loadedChildCount = 0; // How many children have been created so far.
  int totalChildCount = 0;  // Total number available.
  QVector<LazyProtobufNode*> children;

  // --- Constructors ---

  // Constructor for a Message node.
  LazyProtobufNode(const QString& name,
    std::shared_ptr<const google::protobuf::Message> msg,
    LazyProtobufNode* parent = nullptr)
    : type(NodeType::Message), name(name), parent(parent), message(std::move(msg))
  {
    path = (parent ? parent->path + "/" : "") + name;
    if (message) {
      totalChildCount = message->GetDescriptor()->field_count();
    }
  }

  // Constructor for a RepeatedField container node.
  LazyProtobufNode(const QString& name,
    std::shared_ptr<const google::protobuf::Message> parentMsg,
    const google::protobuf::FieldDescriptor* fd,
    LazyProtobufNode* parent = nullptr)
    : type(NodeType::RepeatedField), name(name), parent(parent),
    parentMessage(std::move(parentMsg)), fieldDesc(fd)
  {
    path = (parent ? parent->path + "/" : "") + name;
    if (parentMessage && fieldDesc) {
      totalChildCount = parentMessage->GetReflection()->FieldSize(*parentMessage, fieldDesc);
    }
  }

  // Constructor for a Value (leaf) node.
  LazyProtobufNode(const QString& name, const QVariant& val, LazyProtobufNode* parent = nullptr)
    : type(NodeType::Value), name(name), parent(parent), value(val)
  {
    path = (parent ? parent->path + "/" : "") + name;
    childrenFullyLoaded = true;
    totalChildCount = 0;
  }

  ~LazyProtobufNode() {
    qDeleteAll(children);
    children.clear();
  }
};

//------------------------------------------------------------------------------
// A QAbstractItemModel that builds a tree view of a protobuf message lazily.
class LazyProtobufTreeModel : public QAbstractItemModel {
  Q_OBJECT
public:
  // The model takes a shared_ptr to the root protobuf message.
  LazyProtobufTreeModel(std::shared_ptr<const google::protobuf::Message> rootMsg, QObject* parent = nullptr)
    : QAbstractItemModel(parent)
  {
    rootNode = new LazyProtobufNode("Root", std::move(rootMsg));
  }

  ~LazyProtobufTreeModel() override {
    delete rootNode;
  }

  // Returns the index for the given row/column under parent.
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
    if (!hasIndex(row, column, parent))
      return QModelIndex();

    LazyProtobufNode* parentNode = nodeFromIndex(parent);
    if (row < parentNode->children.size())
      return createIndex(row, column, parentNode->children[row]);
    return QModelIndex();
  }

  // Returns the parent index of a given index.
  QModelIndex parent(const QModelIndex& index) const override {
    if (!index.isValid())
      return QModelIndex();

    LazyProtobufNode* childNode = static_cast<LazyProtobufNode*>(index.internalPointer());
    LazyProtobufNode* parentNode = childNode->parent;
    if (!parentNode || parentNode == rootNode)
      return QModelIndex();

    LazyProtobufNode* grandParent = parentNode->parent;
    int row = (grandParent) ? grandParent->children.indexOf(parentNode) : 0;
    return createIndex(row, 0, parentNode);
  }

  // Returns the number of rows (children) for a given parent.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    LazyProtobufNode* node = nodeFromIndex(parent);
    return node->children.size();
  }

  // We show two columns (e.g. field name and value/type summary).
  int columnCount(const QModelIndex& parent = QModelIndex()) const override {
    Q_UNUSED(parent)
      return 2;
  }

  // Returns display data for a given index.
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
    if (!index.isValid() || role != Qt::DisplayRole)
      return QVariant();

    LazyProtobufNode* node = static_cast<LazyProtobufNode*>(index.internalPointer());
    if (index.column() == 0)
      return node->name;
    else if (index.column() == 1) {
      switch (node->type) {
      case LazyProtobufNode::NodeType::Message:
        return QString("Message");
      case LazyProtobufNode::NodeType::RepeatedField:
        return QString("%1 elements").arg(node->totalChildCount);
      case LazyProtobufNode::NodeType::Value:
        return node->value;
      }
    }
    return QVariant();
  }

  // --- Lazy loading support ---

  // Override hasChildren so that nodes representing messages or repeated fields
  // show an expander even if their children haven't been loaded yet.
  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override {
    LazyProtobufNode* node = nodeFromIndex(parent);
    if (!node)
      return false;
    if (node->type == LazyProtobufNode::NodeType::Value)
      return false;
    return (node->totalChildCount > 0);
  }

  // Determines whether more data can be fetched for the given parent.
  bool canFetchMore(const QModelIndex& parent) const override {
    LazyProtobufNode* node = nodeFromIndex(parent);
    return (node && node->loadedChildCount < node->totalChildCount);
  }

  // Fetches more data (children) for the given parent.
  void fetchMore(const QModelIndex& parent) override {
    LazyProtobufNode* node = nodeFromIndex(parent);
    if (!node)
      return;

    int remainder = node->totalChildCount - node->loadedChildCount;
    int itemsToFetch = std::min(remainder, BATCH_SIZE);
    if (itemsToFetch <= 0)
      return;

    int firstNewRow = node->children.size();
    beginInsertRows(parent, firstNewRow, firstNewRow + itemsToFetch - 1);

    for (int i = 0; i < itemsToFetch; ++i) {
      int fieldIndex = node->loadedChildCount++;
      // --- For a Message node, each field becomes a child ---
      if (node->type == LazyProtobufNode::NodeType::Message) {
        const google::protobuf::Descriptor* desc = node->message->GetDescriptor();
        const google::protobuf::FieldDescriptor* field = desc->field(fieldIndex);
        QString fieldName = QString::fromStdString(field->name());

        // If the field is repeated, create a container node.
        if (field->is_repeated()) {
          LazyProtobufNode* childNode = new LazyProtobufNode(fieldName, node->message, field, node);
          node->children.append(childNode);
        }
        else {
          const google::protobuf::Reflection* refl = node->message->GetReflection();
          // For a sub-message, create a Message node using the aliasing constructor.
          if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            const google::protobuf::Message& subMsgRef = refl->GetMessage(*node->message, field);
            std::shared_ptr<const google::protobuf::Message> subMsgPtr(node->message, &subMsgRef);
            LazyProtobufNode* childNode = new LazyProtobufNode(fieldName, subMsgPtr, node);
            node->children.append(childNode);
          }
          else {
            QVariant val;
            switch (field->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
              val = refl->GetInt32(*node->message, field);
              break;
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
              val = QString::fromStdString(refl->GetString(*node->message, field));
              break;
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
              auto enumVal = refl->GetEnum(*node->message, field);
              val = QString::fromStdString(enumVal->name());
              break;
            }
                                                                // Add additional type cases as needed.
            default:
              val = QString("Unsupported type");
            }
            LazyProtobufNode* childNode = new LazyProtobufNode(fieldName, val, node);
            node->children.append(childNode);
          }
        }
      }
      // --- For a RepeatedField container node, load each element ---
      else if (node->type == LazyProtobufNode::NodeType::RepeatedField) {
        const google::protobuf::Reflection* refl = node->parentMessage->GetReflection();
        const google::protobuf::FieldDescriptor* field = node->fieldDesc;
        int index = node->loadedChildCount - 1; // zero-based index in the repeated field
        QString elementName = QString("Element %1").arg(index);
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
          const google::protobuf::Message& subMsgRef = refl->GetRepeatedMessage(*node->parentMessage, field, index);
          std::shared_ptr<const google::protobuf::Message> subMsgPtr(node->parentMessage, &subMsgRef);
          LazyProtobufNode* childNode = new LazyProtobufNode(elementName, subMsgPtr, node);
          node->children.append(childNode);
        }
        else {
          QVariant val;
          switch (field->cpp_type()) {
          case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            val = refl->GetRepeatedInt32(*node->parentMessage, field, index);
            break;
          case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            val = QString::fromStdString(refl->GetRepeatedString(*node->parentMessage, field, index));
            break;
          case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
            auto enumVal = refl->GetRepeatedEnum(*node->parentMessage, field, index);
            val = QString::fromStdString(enumVal->name());
            break;
          }
                                                              // Add more type cases as needed.
          default:
            val = QString("Unsupported repeated type");
          }
          LazyProtobufNode* childNode = new LazyProtobufNode(elementName, val, node);
          node->children.append(childNode);
        }
      }
    }
    if (node->loadedChildCount >= node->totalChildCount)
      node->childrenFullyLoaded = true;
    endInsertRows();
  }

  // --- Updating the Model with a New Message & Preserving Expansion State ---
  //
  // Before updating, you can call getExpandedPaths() from your view to retrieve the set
  // of node paths (from the root) that are currently expanded.
  //
  // After calling updateMessage(), call restoreExpansionState(view, savedPaths) to re-expand
  // those nodes.
  QSet<QString> getExpandedPaths(QTreeView* view) {
    QSet<QString> expanded;
    storeExpansionState(view, QModelIndex(), expanded);
    return expanded;
  }

  // Restore expansion state in the view given the set of expanded paths.
  void restoreExpansionState(QTreeView* view, const QSet<QString>& expandedPaths, const QModelIndex& parent = QModelIndex()) {
    int rows = rowCount(parent);
    for (int i = 0; i < rows; ++i) {
      QModelIndex childIndex = index(i, 0, parent);
      LazyProtobufNode* node = static_cast<LazyProtobufNode*>(childIndex.internalPointer());
      if (expandedPaths.contains(node->path)) {
        view->expand(childIndex);
        restoreExpansionState(view, expandedPaths, childIndex);
      }
    }
  }

  // Update the model with a new protobuf message.
  // (Make sure to save & later restore the expansion state from your view.)
  void updateMessage(std::shared_ptr<const google::protobuf::Message> newMessage) {
    beginResetModel();
    delete rootNode;
    rootNode = new LazyProtobufNode("Root", std::move(newMessage));
    endResetModel();
  }

private:
  LazyProtobufNode* rootNode = nullptr;

  // Helper: if the index is valid, return its node; otherwise, return the root.
  LazyProtobufNode* nodeFromIndex(const QModelIndex& index) const {
    return index.isValid() ? static_cast<LazyProtobufNode*>(index.internalPointer()) : rootNode;
  }

  // Recursively store expansion state from the view.
  void storeExpansionState(QTreeView* view, const QModelIndex& parent, QSet<QString>& expanded) {
    int rows = rowCount(parent);
    for (int i = 0; i < rows; ++i) {
      QModelIndex childIndex = index(i, 0, parent);
      if (view->isExpanded(childIndex)) {
        LazyProtobufNode* node = static_cast<LazyProtobufNode*>(childIndex.internalPointer());
        expanded.insert(node->path);
        storeExpansionState(view, childIndex, expanded);
      }
    }
  }
};

