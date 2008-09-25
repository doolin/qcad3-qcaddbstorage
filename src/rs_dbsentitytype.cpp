#include "RS_DbsEntityType"
#include "RS_DbCommand"
#include "RS_DbConnection"
#include "RS_DbReader"
    
    
    
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
