#include "RS_Debug"
#include "RS_DbStorage"
#include "RS_DbException"
#include "RS_DbsEntityType"
#include "RS_DbsObjectTypeRegistry"
#include "RS_DbsUcsType"



/**
 * Sets up a DB for the document and initializes the tables.
 *
 * \param fileName File name of DB file or ":memory:" to keep the
 *      DB in memory.
 */
RS_DbStorage::RS_DbStorage(const std::string& fileName) {
    db.open(fileName.c_str());
    
    // 'Transaction' is a reserved keyword, so we use 'Transaction2':
    db.executeNonQuery(
        "CREATE TABLE Transaction2("
            "id INTEGER PRIMARY KEY, "
            "parentId INTEGER, "
            "text VARCHAR"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE AffectedObjects("
            "tid INTEGER, "
            "oid INTEGER, "
            "PRIMARY KEY(tid, oid)"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE PropertyChanges("
            "tid INTEGER, "
            "oid INTEGER, "
            "pid INTEGER, "
            "dataType INTEGER, "
            "oldValue BLOB, "
            "newValue BLOB, "
            "PRIMARY KEY(tid, oid, pid)"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE Variables("
            "key STRING PRIMARY KEY, "
            "value BLOB"
        ");"
    );
    
    RS_DbCommand cmd(
        db, 
        "INSERT INTO Variables VALUES(?,?);"
    );
    cmd.bind(1, "LastTransaction");
    cmd.bind(2, -1);
    cmd.executeNonQuery();
 
    // initialize the DB for all registered object types:
    RS_DbsObjectTypeRegistry::initDb(db);
}



/**
 * Closes the DB connection.
 */
RS_DbStorage::~RS_DbStorage() {
    db.close();
}



void RS_DbStorage::queryAllObjects(std::set<RS_Object::Id>& result) {
    RS_DbsObjectType::queryAllObjects(db, result);
}



void RS_DbStorage::queryAllEntities(std::set<RS_Entity::Id>& result) {
    return RS_DbsEntityType::queryAllEntities(db, result);
}



void RS_DbStorage::queryAllUcs(std::set<RS_Ucs::Id>& result) {
    return RS_DbsUcsType::queryAllUcs(db, result);
}



void RS_DbStorage::querySelectedEntities(std::set<RS_Entity::Id>& result) {
    return RS_DbsEntityType::querySelectedEntities(db, result);
}



RS_Object* RS_DbStorage::queryObject(RS_Object::Id objectId) {
    RS_Object::ObjectTypeId objectTypeId = getObjectTypeId(objectId);
    RS_DbsObjectType* dbsObjectType = RS_DbsObjectTypeRegistry::getDbObject(objectTypeId);
    if (dbsObjectType==NULL) {
        RS_Debug::error("RS_DbStorage::queryObject: "
            "no DB object registered for object type %d", objectTypeId);
        return NULL;
    }

    return dbsObjectType->loadObject(db, objectId);
}



RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId) {
    RS_Object* object = queryObject(entityId);
    if (object==NULL) {
        return NULL;
    }

    RS_Entity* entity = dynamic_cast<RS_Entity*>(object);
    if (entity==NULL) {
        delete object;
        return NULL;
    }
    
    return entity;
}



RS_Ucs* RS_DbStorage::queryUcs(RS_Ucs::Id ucsId) {
    RS_Object* object = queryObject(ucsId);
    if (object==NULL) {
        return NULL;
    }

    RS_Ucs* ucs = dynamic_cast<RS_Ucs*>(object);
    if (ucs==NULL) {
        delete object;
        return NULL;
    }
    
    return ucs;
}



void RS_DbStorage::clearEntitySelection(std::set<RS_Entity::Id>* affectedObjects) {
    RS_DbsEntityType::clearEntitySelection(db, affectedObjects);
}



void RS_DbStorage::selectEntity(
    RS_Entity::Id entityId, bool add, 
    std::set<RS_Entity::Id>* affectedObjects) {

    RS_DbsEntityType::selectEntity(db, entityId, add, affectedObjects);
}



void RS_DbStorage::selectEntities(
    std::set<RS_Entity::Id>& entityIds, 
    bool add, 
    std::set<RS_Entity::Id>* affectedObjects) {
    
    RS_DbsEntityType::selectEntities(db, entityIds, add, affectedObjects);
}



RS_Box RS_DbStorage::getBoundingBox() {
    return RS_DbsEntityType::getBoundingBox(db);
}



void RS_DbStorage::saveObject(RS_Object& object) {
    bool isNew = (object.getId()==-1);

    // look up storage object for this object type in the object type registry:
    RS_DbsObjectType* dbObjectType = 
        RS_DbsObjectTypeRegistry::getDbObject(object.getObjectTypeId());

    // store entity type specific information:
    if (dbObjectType==NULL) {
        RS_Debug::error("RS_DbStorage::saveObject: "
            "no DB storage object registered for object type %d", 
            object.getObjectTypeId());
        return;
    }

    dbObjectType->saveObject(db, object, isNew);
}



void RS_DbStorage::deleteObject(RS_Object::Id objectId) {
    RS_Object::ObjectTypeId objectTypeId = getObjectTypeId(objectId);
    RS_DbsObjectType* dbsObjectType = RS_DbsObjectTypeRegistry::getDbObject(objectTypeId);
    if (dbsObjectType==NULL) {
        RS_Debug::error("RS_DbStorage::deleteObject: "
            "no DB Object registered for object type %d", objectTypeId);
        return;
    }

    // delete record in entity specific table(s) (e.g. from table Line):
    dbsObjectType->deleteObject(db, objectId);
}



void RS_DbStorage::beginTransaction() {
    db.startTransaction();
}



void RS_DbStorage::commitTransaction() {
    db.endTransaction();
}



int RS_DbStorage::getLastTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT value "
        "FROM Variables "
        "WHERE key='LastTransaction'"
    );

    return cmd.executeInt();
}



void RS_DbStorage::setLastTransactionId(int cid) {
    RS_DbCommand cmd(
        db, 
        "UPDATE Variables "
        "SET value=? "
        "WHERE key='LastTransaction'"
    );
    cmd.bind(1, cid);
    cmd.executeNonQuery();
}



void RS_DbStorage::saveTransaction(RS_Transaction& transaction) {
    // if the given transaction is not undoable, we don't need to
    // store anything here:
    if (!transaction.isUndoable()) {
        return;
    }

    // assign new unique ID for this transaction:
    transaction.setId(getLastTransactionId() + 1);

    // delete transactions that are lost for good due to this transaction:
    deleteTransactionsFrom(transaction.getId());
    
    // store the transaction in the transaction log:
    RS_DbCommand cmd(
        db, 
        "INSERT INTO Transaction2 VALUES(?,?,?)"
    );
    cmd.bind(1, transaction.getId());
    cmd.bind(2);
    cmd.bind(3, transaction.getText());
    cmd.executeNonQuery();
    
    RS_Debug::debug("RS_DbStorage::saveTransaction: store transaction: OK");

    // store the set of entities that are affected by the transaction:
    std::set<RS_Object::Id> affectedObjects = transaction.getAffectedObjects();
    std::set<RS_Object::Id>::iterator it;
    for (it=affectedObjects.begin(); it!=affectedObjects.end(); ++it) {
        RS_Debug::debug("RS_DbStorage::saveTransaction: "
            "INSERT INTO AffectedObjects VALUES(%d,%d)", transaction.getId(), *it);

        RS_DbCommand cmd(
            db, 
            "INSERT INTO AffectedObjects VALUES(?,?)"
        );
        cmd.bind(1, transaction.getId());
        cmd.bind(2, *it);
        cmd.executeNonQuery();
        
        RS_Debug::debug("RS_DbStorage::saveTransaction: INSERT: OK");
    }

    // store the property changes for all affected objects:
    std::multimap<RS_Object::Id, RS_PropertyChange> propertyChanges = transaction.getPropertyChanges();
    std::multimap<RS_Object::Id, RS_PropertyChange>::iterator it2;
    for (it2=propertyChanges.begin(); it2!=propertyChanges.end(); ++it2) {
        RS_Debug::debug("RS_DbStorage::saveTransaction: "
            "INSERT INTO PropertyChanges VALUES("
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: transaction ID: "
            "%d",
            transaction.getId()
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: object ID from map: "
            "%d",
            (*it2).first
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: property ID: "
            "%d",
            (*it2).second.propertyTypeId
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: data type: "
            "%d",
            (int)((*it2).second.oldValue.getDataType())
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: old value: "
            "%s",
            (*it2).second.oldValue.getString().c_str()
        );
        RS_Debug::debug("RS_DbStorage::saveTransaction: new value: "
            "%s)",
            (*it2).second.newValue.getString().c_str()
        );

        RS_DbCommand cmd(
            db, 
            "INSERT INTO PropertyChanges VALUES(?,?,?,?,?,?)"
        );
        cmd.bind(1, transaction.getId());
        cmd.bind(2, (*it2).first);
        cmd.bind(3, (*it2).second.propertyTypeId);
        cmd.bind(4, (int)((*it2).second.oldValue.getDataType()));
        // TODO: refactor into RS_PropertyValue::bind (?)
        switch ((*it2).second.oldValue.getDataType()) {
        case RS_PropertyValue::Boolean:
            cmd.bind(5, (*it2).second.oldValue.getBool());
            cmd.bind(6, (*it2).second.newValue.getBool());
            break;
        case RS_PropertyValue::Integer:
            cmd.bind(5, (*it2).second.oldValue.getInt());
            cmd.bind(6, (*it2).second.newValue.getInt());
            break;
        case RS_PropertyValue::Double:
            cmd.bind(5, (*it2).second.oldValue.getDouble());
            cmd.bind(6, (*it2).second.newValue.getDouble());
            break;
        case RS_PropertyValue::String:
            cmd.bind(5, (*it2).second.oldValue.getString());
            cmd.bind(6, (*it2).second.newValue.getString());
            break;
        default:
            break;
        }
        cmd.executeNonQuery();
    }
    
    setLastTransactionId(transaction.getId());
}



RS_Transaction RS_DbStorage::getTransaction(int transactionId) {
    // look up command:
    RS_DbCommand cmd1(
        db, 
        "SELECT text "
        "FROM Transaction2 "
        "WHERE id=?"
    );
    cmd1.bind(1, transactionId);
    
    std::string text;
    try {
        text = cmd1.executeString();
    }
    catch (RS_DbException e) {
        text = "";
    }

    // look up set of affected objects:
    RS_DbCommand cmd2(
        db, 
        "SELECT oid "
        "FROM AffectedObjects "
        "WHERE tid=?"
    );
    cmd2.bind(1, transactionId);

    std::set<RS_Object::Id> affectedObjects;

    RS_DbReader reader = cmd2.executeReader();
    while (reader.read()) {
        affectedObjects.insert(reader.getInt64(0));
        RS_Debug::debug("RS_DbStorage::getTransaction: "
            "affected entity: %d", reader.getInt64(0));
    }

    std::multimap<RS_Object::Id, RS_PropertyChange> propertyChanges;
    
    // load property changes:
    RS_DbCommand cmd3(
        db, 
        "SELECT oid, pid, dataType, oldValue, newValue "
        "FROM PropertyChanges "
        "WHERE tid=?"
    );
    cmd3.bind(1, transactionId);

    reader = cmd3.executeReader();
    while (reader.read()) {
        RS_PropertyChange pc;
        pc.propertyTypeId = reader.getInt64(1);
        switch((RS_PropertyValue::DataType)reader.getInt64(2)) {
        case RS_PropertyValue::Boolean:
            pc.oldValue = RS_PropertyValue((bool)reader.getInt(3));
            pc.newValue = RS_PropertyValue((bool)reader.getInt(4));
            break;
        case RS_PropertyValue::Integer:
            pc.oldValue = RS_PropertyValue(reader.getInt(3));
            pc.newValue = RS_PropertyValue(reader.getInt(4));
            break;
        case RS_PropertyValue::Double:
            pc.oldValue = RS_PropertyValue(reader.getDouble(3));
            pc.newValue = RS_PropertyValue(reader.getDouble(4));
            break;
        case RS_PropertyValue::String:
            pc.oldValue = RS_PropertyValue(reader.getString(3));
            pc.newValue = RS_PropertyValue(reader.getString(4));
            break;
        default:
            RS_Debug::error("RS_DbStorage::getTransaction: "
                "unknown property value type");
            break;
        }
        propertyChanges.insert(std::pair<RS_Object::Id, RS_PropertyChange>(reader.getInt64(0), pc));
    }

    return RS_Transaction(
        *this, 
        transactionId, 
        text, 
        affectedObjects, 
        propertyChanges
    );
}
    
    
    
void RS_DbStorage::deleteTransactionsFrom(int transactionId) {
    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: transactionId: %d", transactionId);

    // delete orphaned objects:
    RS_DbCommand cmd3(
        db, 
        "SELECT oid "
        "FROM AffectedObjects "
        "WHERE tid>=?"
    );
    cmd3.bind(1, transactionId);
    RS_DbReader reader = cmd3.executeReader();
    while (reader.read()) {
        int oid = reader.getInt64(0);
        RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: "
            "check for previous transactions with object %d", oid);

        // check if there are transactions we are keeping which still refer to the
        // entity in question:
        RS_DbCommand cmd4(
            db, 
            "SELECT oid "
            "FROM AffectedObjects "
            "WHERE tid<? AND oid=?"
        );
        cmd4.bind(1, transactionId);
        cmd4.bind(2, oid);
        RS_DbReader reader4 = cmd4.executeReader();
        if (reader4.read()==false) {
            RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: deleteObject: %d", oid);
            deleteObject(oid);
        }
    }
        
    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: "
        "delete records of affected objects");

    // delete records of affected objects for the transactions:
    RS_DbCommand cmd(
        db, 
        "DELETE FROM AffectedObjects "
        "WHERE tid>=?"
    );
    cmd.bind(1, transactionId);
    cmd.executeNonQuery();
    
    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: "
        "delete property changes of transactions");

    // delete property changes for transactions:
    RS_DbCommand cmd5(
        db, 
        "DELETE FROM PropertyChanges "
        "WHERE tid>=?"
    );
    cmd5.bind(1, transactionId);
    cmd5.executeNonQuery();

    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: "
        "delete transaction");
    
    // delete transaction:
    RS_DbCommand cmd2(
        db, 
        "DELETE FROM Transaction2 "
        "WHERE id>=?"
    );
    cmd2.bind(1, transactionId);
    cmd2.executeNonQuery();
    
    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: OK");
}



int RS_DbStorage::getMaxTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT max(id) "
        "FROM Transaction2"
    );
    return cmd.executeInt();
}
    
    
    
void RS_DbStorage::toggleUndoStatus(std::set<RS_Object::Id>& objects) {
    std::set<RS_Object::Id>::iterator it;
    for (it=objects.begin(); it!=objects.end(); it++) {
        RS_Debug::debug("RS_DbStorage::undo: toggle object: %d", *it);

        toggleUndoStatus(*it);
    }
}



void RS_DbStorage::toggleUndoStatus(RS_Object::Id objectId) {
    RS_DbCommand cmd(
        db, 
        "UPDATE Object "
        "SET undoStatus=NOT(undoStatus) "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();
}



bool RS_DbStorage::getUndoStatus(RS_Object::Id objectId) {
    RS_DbCommand cmd(
        db, 
        "SELECT undoStatus "
        "FROM Object "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    return (cmd.executeInt()!=0);
}



/**
 * \return Object type ID of the given object or RS_Object::UnknownObject if 
 *      the object does not exist.
 */
RS_Object::ObjectTypeId RS_DbStorage::getObjectTypeId(RS_Object::Id objectId) {
    // query object type ID:
    RS_DbCommand cmd(
        db, 
        "SELECT objectTypeId "
        "FROM Object "
        "WHERE id=? "
        "  AND undoStatus=0"
    );
    cmd.bind(1, objectId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        return RS_Object::UnknownObject;
    }

    return (RS_Object::ObjectTypeId)reader.getInt64(0);
}



/**
 * Helper function that turns the given list of IDs into an SQL
 * string list.
 *
 * \return List of IDs as string for use in SQL queries. 
 *      E.g. "(1,7,5,17)"
 */
std::string RS_DbStorage::getSqlList(std::set<RS_Object::Id>& values) {
    std::stringstream ss;
    ss << "(";
    std::set<RS_Object::Id>::iterator it;
    for (it=values.begin(); it!=values.end(); ++it) {
        if (it!=values.begin()) {
            ss << ",";
        }
        ss << (*it);
    }
    ss << ")";
    return ss.str();
}
