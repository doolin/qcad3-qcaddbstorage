#include "RS_Debug"
#include "RS_DbStorage"
#include "RS_DbException"
#include "RS_DbsEntityType"
#include "RS_DbsEntityTypeRegistry"



/**
 * Sets up a DB for the document and initializes the tables.
 *
 * \param fileName File name of DB file or ":memory:" to keep the
 *      DB in memory.
 */
RS_DbStorage::RS_DbStorage(const std::string& fileName) {
    db.open(fileName.c_str());
    
    // there is no header for shell.c, so this cannot work: 
    //struct callback_data data;
    //do_meta_command(".dump", &data);
    
    db.executeNonQuery(
        "CREATE TABLE Object("
            "id INTEGER PRIMARY KEY, "
            "objectType INTEGER, "
            "undoStatus INTEGER"
        ");"
    );

    db.executeNonQuery(
        "CREATE TABLE Entity("
            "id INTEGER PRIMARY KEY, "
            "entityType INTEGER, "
            "selectionStatus INTEGER, "
            "minX REAL, "
            "minY REAL, "
            "minZ REAL, "
            "maxX REAL, "
            "maxY REAL, "
            "maxZ REAL"
        ");"
    );

    /*
    db.executeNonQuery(
        "CREATE TABLE Block("
            "name VARCHAR PRIMARY KEY"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE Layer("
            "id INTEGER PRIMARY KEY, "
            "originalId INTEGER, "
            "name VARCHAR PRIMARY KEY"
        ");"
    );
    */
    
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
        "CREATE TABLE LastTransaction("
            "lastTransaction INTEGER PRIMARY KEY"
        ");"
    );
    
    RS_DbCommand cmd(
        db, 
        "INSERT INTO LastTransaction VALUES(?);"
    );
    cmd.bind(1, -1);
    cmd.executeNonQuery();
 
    RS_DbsEntityTypeRegistry::initDb(db);
}



/**
 * Closes the DB connection.
 */
RS_DbStorage::~RS_DbStorage() {
    db.close();
}



std::set<RS_Entity::Id> RS_DbStorage::queryAll() {
    std::set<RS_Entity::Id> ret;
            
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Entity"
    );

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



std::set<RS_Entity::Id> RS_DbStorage::querySelected() {
    std::set<RS_Entity::Id> ret;
            
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Entity "
        "WHERE selectionStatus=1"
    );

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



RS_Object* RS_DbStorage::queryObject(RS_Object::Id objectId) {
    // query object type:
    RS_DbCommand cmd(
        db, 
        "SELECT objectType "
        "FROM Object "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        return NULL;
    }

    switch((RS_Object::ObjectTypeId)reader.getInt64(0)) {
    case RS_Object::EntityObject:
        return (RS_Object*)queryEntity(objectId);
    // TODO: implement more objet types (layers, UCS, ...)
    default:
        return NULL;
    }
}



RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId) {
    // query entity type:
    RS_DbCommand cmd(
        db, 
        "SELECT entityType "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        return NULL;
    }

    RS_Entity::EntityTypeId typeId = (RS_Entity::EntityTypeId)reader.getInt64(0);
        
    return queryEntity(entityId, typeId);
}
    
    

/**
 * Internal function to query an entity if the type is already known.
 */
RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId, RS_Entity::EntityTypeId typeId) {
    RS_DbsEntityType* dbsEntityType = RS_DbsEntityTypeRegistry::getDbEntity(typeId);
    if (dbsEntityType==NULL) {
        RS_Debug::error("RS_DbStorage::queryEntity: "
            "no DB Entity object registered for entity type %d", typeId);
        return NULL;
    }
    return dbsEntityType->instantiate(db, entityId);
}



void RS_DbStorage::clearSelection(std::set<RS_Entity::Id>* affectedObjects) {
    // find out which entities will be affected:
    if (affectedObjects!=NULL) {
        RS_DbCommand cmd(
            db, 
            "SELECT id "
            "FROM Entity "
            "WHERE selectionStatus=1"
        );
        RS_DbReader reader = cmd.executeReader();
        while (reader.read()) {
            affectedObjects->insert(reader.getInt64(0));
        }
    }

    RS_DbCommand cmd(
        db, 
        "UPDATE Entity "
        "SET selectionStatus=0 "
        "WHERE selectionStatus=1"
    );
    cmd.executeNonQuery();
}



void RS_DbStorage::selectEntity(
    RS_Entity::Id entityId, bool add, 
    std::set<RS_Entity::Id>* affectedObjects) {

    if (add) {
        // only the entity that is added to the selection is affected:
        if (affectedObjects!=NULL) {
            affectedObjects->insert(entityId);
        }

        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=1 "
            "WHERE id=?"
        );
        cmd.bind(1, entityId);
        cmd.executeNonQuery();
    }
    else {
        // find out which entities will be affected:
        if (affectedObjects!=NULL) {
            RS_DbCommand cmd(
                db, 
                "SELECT id "
                "FROM Entity "
                "WHERE (id=? AND selectionStatus=0) OR "
                "      (id!=? AND selectionStatus=1)"
            );
            cmd.bind(1, entityId);
            cmd.bind(2, entityId);
            RS_DbReader reader = cmd.executeReader();
            while (reader.read()) {
                affectedObjects->insert(reader.getInt64(0));
            }
        }

        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=NOT(selectionStatus) "
            "WHERE (id=? AND selectionStatus=0) OR "
            "      (id!=? AND selectionStatus=1)"
        );
        cmd.bind(1, entityId);
        cmd.bind(2, entityId);
        cmd.executeNonQuery();
    }
}



void RS_DbStorage::selectEntities(
    std::set<RS_Entity::Id>& entityIds, 
    bool add, 
    std::set<RS_Entity::Id>* affectedObjects) {
        
    if (affectedObjects!=NULL) {
        (*affectedObjects) = entityIds;
    }

    //if (add) {
        // only the entities that are added to the selection are affected:
        //if (affectedObjects!=NULL) {
            //affectedObjects->insert(affectedObjects->begin(),myvector.begin(),myvector.end());
            //affectedObjects->insert(entityId);
        //    (*affectedObjects) = entityIds;
        //}

        /*
        RS_DbCommand cmd(
            db, 
            std::string(
                "UPDATE Entity "
                "SET selectionStatus=1 "
                "WHERE id IN "
            ) + getSqlList(entityIds)
        );
        //cmd.bind(1, entityId);
        cmd.executeNonQuery();
        */
    //}
    //else {
    if (!add) {
        // find out which entities will be deselected:
        if (affectedObjects!=NULL) {
            //(*affectedObjects) = entityIds;

            RS_DbCommand cmd(
                db, 
                "SELECT id "
                "FROM Entity "
                "WHERE selectionStatus=1"
            );
            RS_DbReader reader = cmd.executeReader();
            while (reader.read()) {
                affectedObjects->insert(reader.getInt64(0));
            }
        }

        // deselect all:
        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=0 "
            "WHERE selectionStatus=1"
        );
        cmd.executeNonQuery();
    }

    // select given entities:
    RS_DbCommand cmd(
        db, 
        std::string(
            "UPDATE Entity "
            "SET selectionStatus=1 "
            "WHERE id IN "
        ) + getSqlList(entityIds)
    );
    cmd.executeNonQuery();
}



RS_Box RS_DbStorage::getBoundingBox() {
    RS_DbCommand cmd(
        db, 
        "SELECT MIN(minX), MIN(minY), MIN(minZ), "
        "       MAX(maxX), MAX(maxY), MAX(maxZ) "
        "FROM Object, Entity "
        "WHERE Object.id=Entity.id "
        "   AND undoStatus=0"
    );
    RS_DbReader reader = cmd.executeReader();

    RS_Vector minV;
    RS_Vector maxV;
    
    if (reader.read()) {
        minV.x = reader.getDouble(0);
        minV.y = reader.getDouble(1);
        minV.z = reader.getDouble(2);
        
        maxV.x = reader.getDouble(3);
        maxV.y = reader.getDouble(4);
        maxV.z = reader.getDouble(5);
    }

    return RS_Box(minV, maxV);
}



int RS_DbStorage::getLastTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT lastTransaction "
        "FROM LastTransaction"
    );

    return cmd.executeInt();
}



void RS_DbStorage::setLastTransactionId(int cid) {
    RS_DbCommand cmd(
        db, 
        "UPDATE LastTransaction "
        "SET lastTransaction=?"
    );
    cmd.bind(1, cid);
    cmd.executeNonQuery();
}
    
    
    
void RS_DbStorage::saveObject(RS_Object& object) {
    bool isNew = (object.getId()==-1);

    // new object:
    if (isNew) {
        // generic object information has to be stored for all object types:
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Object VALUES(?,?,?);"
        );

        cmd.bind(1);
        cmd.bind(2, object.getObjectTypeId());
        cmd.bind(3, 0);

        cmd.executeNonQuery();
        object.setId(db.getLastInsertedRowId());
    }
    // existing object:
    else {
        // nothing to update..
        /*
        RS_DbCommand cmd(
            db, 
            "UPDATE Object SET undoStatus=? "
            "WHERE id=?"
        );

        cmd.bind(1, object.);
        cmd.bind(2, object.getObjectTypeId());
        cmd.bind(3, 0);

        cmd.executeNonQuery();
        object.setId(db.getLastInsertedRowId());
        */
    }

    switch (object.getObjectTypeId()) {
    case RS_Object::EntityObject:
        saveEntity(dynamic_cast<RS_Entity&>(object), isNew);
        break;
    
    default:
        break;
    }
}



void RS_DbStorage::saveEntity(RS_Entity& entity, bool isNew) {
    RS_Box boundingBox = entity.getBoundingBox();
    RS_Vector c1 = boundingBox.getDefiningCorner1();
    RS_Vector c2 = boundingBox.getDefiningCorner2();

    if (isNew) {
        // generic entity information has to be stored for all entity types:
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Entity VALUES(?,?,?,?,?,?,?,?,?);"
        );
                
        // ID (was set automatically by saveObject()):
        cmd.bind(1, entity.getId());
        cmd.bind(2, entity.getEntityTypeId());   // entityType
        cmd.bind(3, entity.isSelected());        // selectionStatus
        cmd.bind(4, c1.x);                       // minX
        cmd.bind(5, c1.y);                       // minY
        cmd.bind(6, c1.z);                       // minZ
        cmd.bind(7, c2.x);                       // maxX
        cmd.bind(8, c2.y);                       // maxY
        cmd.bind(9, c2.z);                       // maxZ

        cmd.executeNonQuery();
    }
    else {
        RS_DbCommand cmd(
            db, 
            "UPDATE Entity SET selectionStatus=?, minX=?, minY=?, minZ=?, maxX=?, maxY=?, maxZ=? "
            "WHERE id=?"
        );
                
        cmd.bind(1, entity.isSelected());        // selectionStatus
        cmd.bind(2, c1.x);                       // minX
        cmd.bind(3, c1.y);                       // minY
        cmd.bind(4, c1.z);                       // minZ
        cmd.bind(5, c2.x);                       // maxX
        cmd.bind(6, c2.y);                       // maxY
        cmd.bind(7, c2.z);                       // maxZ
        cmd.bind(8, entity.getId());

        cmd.executeNonQuery();
    }
    
    // look up storage object for this entity type in the entity type registry:
    RS_DbsEntityType* dbEntityType = 
        RS_DbsEntityTypeRegistry::getDbEntity(entity.getEntityTypeId());

    // store entity type specific information:
    if (dbEntityType!=NULL) {
        dbEntityType->save(db, entity, isNew);
    }
    else {
        RS_Debug::error("RS_DbStorage::saveEntity: "
            "no DB storage object registered for entity type %d", 
            entity.getEntityTypeId());
    }
}
    
    
    
void RS_DbStorage::beginTransaction() {
    db.startTransaction();
}



void RS_DbStorage::commitTransaction() {
    db.endTransaction();
}
    
    
    
void RS_DbStorage::saveTransaction(RS_Transaction& transaction) {
    transaction.setId(getLastTransactionId() + 1);
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

    // store the set of entities that are affected by the transaction:
    std::set<RS_Entity::Id> affectedObjects = transaction.getAffectedObjects();
    std::set<RS_Entity::Id>::iterator it;
    for (it=affectedObjects.begin(); it!=affectedObjects.end(); ++it) {
        RS_DbCommand cmd(
            db, 
            "INSERT INTO AffectedObjects VALUES(?,?);"
        );
        cmd.bind(1, transaction.getId());
        cmd.bind(2, *it);
        cmd.executeNonQuery();
    
        // store the property changes for all affected objects:
        std::multimap<RS_Object::Id, RS_PropertyChange> propertyChanges = transaction.getPropertyChanges();
        std::multimap<RS_Object::Id, RS_PropertyChange>::iterator it2;
        for (it2=propertyChanges.begin(); it2!=propertyChanges.end(); ++it2) {
            RS_DbCommand cmd2(
                db, 
                "INSERT INTO PropertyChanges VALUES(?,?,?,?,?,?);"
            );
            cmd2.bind(1, transaction.getId());
            cmd2.bind(2, *it);
            cmd2.bind(3, (*it2).second.propertyTypeId);
            cmd2.bind(4, (int)((*it2).second.oldValue.getDataType()));
            // TODO: refactor into RS_PropertyValue::bind (?)
            switch ((*it2).second.oldValue.getDataType()) {
            case RS_PropertyValue::Boolean:
                cmd2.bind(5, (*it2).second.oldValue.getBool());
                cmd2.bind(6, (*it2).second.newValue.getBool());
                break;
            case RS_PropertyValue::Integer:
                cmd2.bind(5, (*it2).second.oldValue.getInt());
                cmd2.bind(6, (*it2).second.newValue.getInt());
                break;
            case RS_PropertyValue::Double:
                cmd2.bind(5, (*it2).second.oldValue.getDouble());
                cmd2.bind(6, (*it2).second.newValue.getDouble());
                break;
            case RS_PropertyValue::String:
                cmd2.bind(5, (*it2).second.oldValue.getString());
                cmd2.bind(6, (*it2).second.newValue.getString());
                break;
            default:
                break;
            }
            cmd2.executeNonQuery();
        }
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

    // delete orphaned entities:
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
            RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: deleteEntity: %d", oid);
            // TODO: deleteObject
            deleteEntity(oid);
        }
    }

    // delete records of affected entities of the command:
    RS_DbCommand cmd(
        db, 
        "DELETE FROM AffectedObjects "
        "WHERE tid>=?"
    );
    cmd.bind(1, transactionId);
    cmd.executeNonQuery();
    
    // delete transaction:
    RS_DbCommand cmd2(
        db, 
        "DELETE FROM Transaction2 "
        "WHERE id>=?"
    );
    cmd2.bind(1, transactionId);
    cmd2.executeNonQuery();
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
    
    
    
void RS_DbStorage::deleteEntity(RS_Entity::Id entityId) {
    RS_Entity::EntityTypeId typeId = getEntityType(entityId);

    RS_DbsEntityTypeRegistry::deleteEntity(db, typeId, entityId);
    
    RS_DbCommand cmd1(
        db, 
        "DELETE FROM Object "
        "WHERE id=?"
    );
    cmd1.bind(1, entityId);
    cmd1.executeNonQuery();

    RS_DbCommand cmd2(
        db, 
        "DELETE FROM Entity "
        "WHERE id=?"
    );
    cmd2.bind(1, entityId);
    cmd2.executeNonQuery();
}
    

    
RS_Entity::EntityTypeId RS_DbStorage::getEntityType(RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT type "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    return cmd.executeInt();
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
