#include "RS_DbsEntityType"
#include "RS_DbCommand"
#include "RS_DbConnection"
#include "RS_DbReader"
#include "RS_DbStorage"
    
    
    
void RS_DbsEntityType::initDb(RS_DbConnection& db) {
    RS_DbsObjectType::initDb(db);

    db.executeNonQuery(
        "CREATE TABLE IF NOT EXISTS Entity("
            "id INTEGER PRIMARY KEY, "
            //"entityType INTEGER, "
            "selectionStatus INTEGER, "
            "minX REAL, "
            "minY REAL, "
            "minZ REAL, "
            "maxX REAL, "
            "maxY REAL, "
            "maxZ REAL"
        ");"
    );
}



void RS_DbsEntityType::loadObject(
    RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId) {

    RS_DbsObjectType::loadObject(db, object, objectId);
    
    RS_Entity* entity = dynamic_cast<RS_Entity*>(&object);
    if (entity==NULL) {
        RS_Debug::error("RS_DbsEntityType::loadObject: given object not an entity");
        return;
    }
    
    //RS_DbsObjectType::loadEntityData(db, data, objectId);

    RS_DbCommand cmd(
        db, 
        "SELECT selectionStatus "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbsEntityType::readEntityData: "
            "cannot read data for entity %d", objectId);
        return;
    }

    entity->setSelected(reader.getInt(0)!=0);
    //data.selectionStatus = (reader.getInt(0)!=0);
    //RS_Debug::debug("RS_DbsEntityType::readEntityData: %d", data.selectionStatus);
}



void RS_DbsEntityType::saveObject(RS_DbConnection& db, RS_Object& object, bool isNew) {
    RS_DbsObjectType::saveObject(db, object, isNew);
    
    RS_Entity* entity = dynamic_cast<RS_Entity*>(&object);
    if (entity==NULL) {
        return;
    }

    RS_Box boundingBox = entity->getBoundingBox();
    RS_Vector c1 = boundingBox.getDefiningCorner1();
    RS_Vector c2 = boundingBox.getDefiningCorner2();

    if (isNew) {
        // generic entity information has to be stored for all entity types:
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Entity VALUES(?,?,?,?,?,?,?,?);"
        );
                
        // ID (was set automatically by saveObject()):
        cmd.bind(1, entity->getId());
        //cmd.bind(2, entity.getEntityTypeId());   // entityType
        cmd.bind(2, entity->isSelected());        // selectionStatus
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
                
        cmd.bind(1, entity->isSelected());        // selectionStatus
        cmd.bind(2, c1.x);                       // minX
        cmd.bind(3, c1.y);                       // minY
        cmd.bind(4, c1.z);                       // minZ
        cmd.bind(5, c2.x);                       // maxX
        cmd.bind(6, c2.y);                       // maxY
        cmd.bind(7, c2.z);                       // maxZ
        cmd.bind(8, entity->getId());

        cmd.executeNonQuery();
    }
}



void RS_DbsEntityType::deleteObject(RS_DbConnection& db, RS_Object::Id objectId) {
    // delete record in Entity table:
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();
    
    RS_DbsObjectType::deleteObject(db, objectId);
}



/**
 * Helper function for RS_DbStorage.
 */
void RS_DbsEntityType::queryAllEntities(RS_DbConnection& db, std::set<RS_Entity::Id>& result) {
    RS_DbCommand cmd(
        db, 
        "SELECT Object.id "
        "FROM Object, Entity "
        "WHERE Object.undoStatus=0 "
        "  AND Object.id=Entity.id"
    );

    RS_DbReader reader = cmd.executeReader();
    while (reader.read()) {
        result.insert(reader.getInt64(0));
    }
}
    
    
    
/**
 * Helper function for RS_DbStorage.
 */
void RS_DbsEntityType::querySelectedEntities(RS_DbConnection& db, std::set<RS_Object::Id>& result) {
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
        result.insert(reader.getInt64(0));
    }
}
    
    
    
/**
 * Helper function for RS_DbStorage.
 */
void RS_DbsEntityType::clearEntitySelection(RS_DbConnection& db, std::set<RS_Entity::Id>* affectedObjects) {
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



/**
 * Helper function for RS_DbStorage.
 */
void RS_DbsEntityType::selectEntity(
    RS_DbConnection& db,
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



/**
 * Helper function for RS_DbStorage.
 */
void RS_DbsEntityType::selectEntities(
    RS_DbConnection& db,
    std::set<RS_Entity::Id>& entityIds, 
    bool add, 
    std::set<RS_Entity::Id>* affectedObjects) {
    
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
        ) + RS_DbStorage::getSqlList(entityIds)
    );
    cmd.executeNonQuery();
}



/**
 * Helper function for RS_DbStorage.
 */
RS_Box RS_DbsEntityType::getBoundingBox(RS_DbConnection& db) {
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
