#include "RS_Debug"
#include "RS_DbStorage"
#include "RS_DbException"
#include "RS_DbsEntityType"
#include "RS_DbsObjectTypeRegistry"



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
 
    RS_DbsObjectTypeRegistry::initDb(db);
}



/**
 * Closes the DB connection.
 */
RS_DbStorage::~RS_DbStorage() {
    db.close();
}



std::set<RS_Object::Id> RS_DbStorage::queryAllObjects() {
    // TODO: move to RS_DbsObjectType
    std::set<RS_Object::Id> ret;
            
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Object "
        "WHERE undoStatus=0"
    );

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



std::set<RS_Entity::Id> RS_DbStorage::queryAllEntities() {
    // TODO: move to RS_DbsEntityType
    std::set<RS_Entity::Id> ret;
            
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Object "
        "WHERE undoStatus=0 "
        "  AND objectType>=100"
    );
    //cmd.bind(1, RS_Object::EntityObject);

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



std::set<RS_Entity::Id> RS_DbStorage::querySelectedEntities() {
    // TODO: move to RS_DbsEntityType
    std::set<RS_Entity::Id> ret;
            
    RS_DbCommand cmd(
        db, 
        "SELECT Object.id "
        "FROM Object, Entity "
        "WHERE Object.id=Entity.id "
        "  AND Object.undoStatus=0 "
        "  AND Entity.selectionStatus=1"
    );

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



RS_Object* RS_DbStorage::queryObject(RS_Object::Id objectId) {
    RS_Object::ObjectTypeId objectTypeId = getObjectTypeId(objectId);
    return queryObject(objectId, objectTypeId);

    /*
    switch (objectTypeId) {
    //case RS_Object::EntityObject:
    //    return (RS_Object*)queryEntity(objectId);
    
    case RS_Object::UcsObject:
        return queryUcs(objectId);

    default:
        return queryEntity(objectId, objectTypeId);
    }
    */
}



/*
RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId) {
    // query entity type:
    RS_DbCommand cmd(
        db, 
        "SELECT Entity.entityType "
        "FROM Object, Entity "
        "WHERE Object.id=Entity.id "
        "  AND Object.id=? "
        "  AND Object.objectType=? "
        "  AND Object.undoStatus=0"
    );
    cmd.bind(1, entityId);
    cmd.bind(2, RS_Object::EntityObject);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbStorage::queryEntity: "
            "cannot read data for entity %d", entityId);
        return NULL;
    }

    RS_Entity::EntityTypeId typeId = (RS_Entity::EntityTypeId)reader.getInt64(0);
        
    return queryEntity(entityId, typeId);
}
*/



/*
RS_Ucs* RS_DbStorage::queryUcs(RS_Ucs::Id ucsId) {
    RS_Vector origin;
    RS_Vector xAxisDirection;
    RS_Vector yAxisDirection;

    RS_DbCommand cmd(
        db, 
        "SELECT originX,originY,originZ, "
        "       xAxisDirectionX,xAxisDirectionY,xAxisDirectionZ, "
        "       yAxisDirectionX,yAxisDirectionY,yAxisDirectionZ "
        "FROM Ucs "
        "WHERE id=?"
    );
    cmd.bind(1, ucsId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbStorage::queryUcs: "
            "cannot read data for UCS %d", ucsId);
        return NULL;
    }

    origin.x = reader.getDouble(0);
    origin.y = reader.getDouble(1);
    origin.z = reader.getDouble(2);
    
    xAxisDirection.x = reader.getDouble(3);
    xAxisDirection.y = reader.getDouble(4);
    xAxisDirection.z = reader.getDouble(5);
    
    yAxisDirection.x = reader.getDouble(6);
    yAxisDirection.y = reader.getDouble(7);
    yAxisDirection.z = reader.getDouble(8);

    RS_Ucs* ucs = new RS_Ucs(origin, xAxisDirection, yAxisDirection);
    ucs->setId(ucsId);

    return ucs;
}
*/



/**
 * Internal function to query an entity if the type is already known.
 */
RS_Object* RS_DbStorage::queryObject(RS_Object::Id objectId, RS_Object::ObjectTypeId objectTypeId) {
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
    // TODO: move to RS_DbsEntityType
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
    // TODO: move to RS_DbsEntityType

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
    // TODO: move to RS_DbsEntityType
        
    if (affectedObjects!=NULL) {
        (*affectedObjects) = entityIds;
    }

    if (!add) {
        // find out which entities will be deselected:
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
    // TODO: move to RS_DbsEntityType
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



void RS_DbStorage::saveObject(RS_Object& object) {
    bool isNew = (object.getId()==-1);

    /*
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
        // nothing to update at this level.
    }
    */
    
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

    /*
    switch (object.getObjectTypeId()) {
    case RS_Object::EntityObject:
        saveEntity(dynamic_cast<RS_Entity&>(object), isNew);
        break;

    case RS_Object::UcsObject:
        saveUcs(dynamic_cast<RS_Ucs&>(object), isNew);
        break;
    
    default:
        break;
    }
    */
}



/**
 * Internal function that stores entity objects. This is called
 * from \ref saveObject.
 */
/*
void RS_DbStorage::saveEntity(RS_Entity& entity, bool isNew) {
    RS_Box boundingBox = entity.getBoundingBox();
    RS_Vector c1 = boundingBox.getDefiningCorner1();
    RS_Vector c2 = boundingBox.getDefiningCorner2();

    if (isNew) {
        // generic entity information has to be stored for all entity types:
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Entity VALUES(?,?,?,?,?,?,?,?);"
        );
                
        // ID (was set automatically by saveObject()):
        cmd.bind(1, entity.getId());
        //cmd.bind(2, entity.getEntityTypeId());   // entityType
        cmd.bind(2, entity.isSelected());        // selectionStatus
        cmd.bind(3, c1.x);                       // minX
        cmd.bind(4, c1.y);                       // minY
        cmd.bind(5, c1.z);                       // minZ
        cmd.bind(6, c2.x);                       // maxX
        cmd.bind(7, c2.y);                       // maxY
        cmd.bind(8, c2.z);                       // maxZ

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
        RS_DbsObjectTypeRegistry::getDbEntity(entity.getObjectTypeId());

    // store entity type specific information:
    if (dbEntityType==NULL) {
        RS_Debug::error("RS_DbStorage::saveEntity: "
            "no DB storage object registered for object / entity type %d", 
            entity.getObjectTypeId());
        return;
    }

    dbEntityType->save(db, entity, isNew);
}
*/



/**
 * Internal function that stores UCS objects. This is called
 * from \ref saveObject.
 */
/*
void RS_DbStorage::saveUcs(RS_Ucs& ucs, bool isNew) {
    if (isNew) {
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Ucs "
            "VALUES(?, ?,?,?, ?,?,?, ?,?,?);"
        );
                
        // ID (was set automatically by saveObject()):
        cmd.bind( 1, ucs.getId());
        cmd.bind( 2, ucs.origin.x);
        cmd.bind( 3, ucs.origin.y);
        cmd.bind( 4, ucs.origin.z);
        cmd.bind( 5, ucs.xAxisDirection.x);
        cmd.bind( 6, ucs.xAxisDirection.y);
        cmd.bind( 7, ucs.xAxisDirection.z);
        cmd.bind( 8, ucs.yAxisDirection.x);
        cmd.bind( 9, ucs.yAxisDirection.y);
        cmd.bind(10, ucs.yAxisDirection.z);

        cmd.executeNonQuery();
    }
    else {
        RS_DbCommand cmd(
            db, 
            "UPDATE Ucs "
            "SET originX=?, originY=?, originZ=?, "
            "    xAxisDirectionX=?, xAxisDirectionY=?, xAxisDirectionZ=?, "
            "    yAxisDirectionX=?, yAxisDirectionY=?, yAxisDirectionZ=? "
            "WHERE id=?"
        );
        
        cmd.bind( 1, ucs.origin.x);
        cmd.bind( 2, ucs.origin.y);
        cmd.bind( 3, ucs.origin.z);
        cmd.bind( 4, ucs.xAxisDirection.x);
        cmd.bind( 5, ucs.xAxisDirection.y);
        cmd.bind( 6, ucs.xAxisDirection.z);
        cmd.bind( 7, ucs.yAxisDirection.x);
        cmd.bind( 8, ucs.yAxisDirection.y);
        cmd.bind( 9, ucs.yAxisDirection.z);
        cmd.bind(10, ucs.getId());

        cmd.executeNonQuery();
    }
}
*/



void RS_DbStorage::deleteObject(RS_Object::Id objectId) {
    /*
    switch(getObjectTypeId(objectId)) {
    case RS_Object::EntityObject:
        deleteEntity(objectId);
        break;
    
    case RS_Object::UcsObject:
        deleteUcs(objectId);
        break;

    default:
        break;
    }
    
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Object "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();
    */
    
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



/**
 * Internal function that deletes entity objects. This is called
 * from \ref deleteObject.
 */
/*
void RS_DbStorage::deleteEntity(RS_Entity::Id entityId) {
    RS_Debug::debug("RS_DbStorage::deleteEntity: %d", entityId);

    RS_Object::ObjectTypeId objectTypeId = getObjectTypeId(entityId);

    if (objectTypeId==-1) {
        RS_Debug::debug("RS_DbStorage::deleteEntity: entity not found");
        return;
    }

    //RS_DbsObjectTypeRegistry::deleteEntity(db, typeId, entityId);
    RS_DbsEntityType* dbsEntityType = RS_DbsObjectTypeRegistry::getDbEntity(objectTypeId);
    if (dbsEntityType==NULL) {
        RS_Debug::error("RS_DbStorage::deleteEntity: "
            "no DB Entity object registered for entity type %d", objectTypeId);
        return;
    }

    // delete record in entity specific table(s) (e.g. from table Line):
    dbsEntityType->deleteEntity(db, entityId);
    
    // delete record in Entity table:
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    cmd.executeNonQuery();
}
*/



/**
 * Internal function that deletes UCS objects. This is called
 * from \ref deleteObject.
 */
/*
void RS_DbStorage::deleteUcs(RS_Ucs::Id ucsId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Ucs "
        "WHERE id=?"
    );
    cmd.bind(1, ucsId);
    cmd.executeNonQuery();
}
*/



void RS_DbStorage::beginTransaction() {
    db.startTransaction();
}



void RS_DbStorage::commitTransaction() {
    db.endTransaction();
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



void RS_DbStorage::saveTransaction(RS_Transaction& transaction) {
    transaction.setId(getLastTransactionId() + 1);

    RS_Debug::debug("RS_DbStorage::saveTransaction: %d", transaction.getId());
    
    deleteTransactionsFrom(transaction.getId());
    
    RS_Debug::debug("RS_DbStorage::saveTransaction: store transaction");

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
        "SELECT objectType "
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
 * \return Entity type ID of the given entity or -1 if the entity does not exist.
 */
/*
RS_Entity::EntityTypeId RS_DbStorage::getEntityTypeId(RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT entityType "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        return -1;
    }
    else {
        return reader.getInt64(0);
    }
}
*/



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
