#include "RS_DbsUcsType"
#include "RS_DbClient"
#include "RS_Ucs"
#include "RS_DbsObjectTypeRegistry"



void RS_DbsUcsType::registerType() {
    RS_DbsObjectTypeRegistry::registerObjectType(
        RS_Ucs::getObjectTypeIdStatic(), 
        new RS_DbsUcsType()
    );
}



void RS_DbsUcsType::initDb(RS_DbConnection& db) {
    RS_DbsObjectType::initDb(db);
    
    db.executeNonQuery(
        "CREATE TABLE Ucs("
            "id INTEGER PRIMARY KEY, "
            "originX REAL, "
            "originY REAL, "
            "originZ REAL, "
            "xAxisDirectionX REAL, "
            "xAxisDirectionY REAL, "
            "xAxisDirectionZ REAL, "
            "yAxisDirectionX REAL, "
            "yAxisDirectionY REAL, "
            "yAxisDirectionZ REAL"
        ");"
    );
}



RS_Object* RS_DbsUcsType::loadObject(RS_DbConnection& db, RS_Object::Id objectId) {
    RS_Ucs* ucs = new RS_Ucs();
    loadObject(db, *ucs, objectId);
    return ucs;
}



void RS_DbsUcsType::loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId) {
    RS_DbsObjectType::loadObject(db, object, objectId);

    RS_Ucs* ucs = dynamic_cast<RS_Ucs*>(&object);
    if (ucs==NULL) {
        RS_Debug::error("RS_DbsUcsType::loadObject: given object not a ucs");
        return;
    }

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
    cmd.bind(1, objectId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbStorage::queryUcs: "
            "cannot read data for UCS %d", objectId);
        return;
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

    ucs->setOrigin(origin);
    ucs->setXAxisDirection(xAxisDirection);
    ucs->setYAxisDirection(yAxisDirection);
}



void RS_DbsUcsType::saveObject(RS_DbConnection& db, RS_Object& object, bool isNew) {
    RS_DbsObjectType::saveObject(db, object, isNew);

    RS_Ucs* ucs = dynamic_cast<RS_Ucs*>(&object);
    if (ucs==NULL) {
        RS_Debug::error("RS_DbsUcsType::saveObject: given object not a UCS");
        return;
    }

    // add ucs as new entity:
    if (isNew) {
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Ucs "
            "VALUES(?, ?,?,?, ?,?,?, ?,?,?);"
        );
                
        // ID (was set automatically by saveObject()):
        cmd.bind( 1, ucs->getId());
        cmd.bind( 2, ucs->origin.x);
        cmd.bind( 3, ucs->origin.y);
        cmd.bind( 4, ucs->origin.z);
        cmd.bind( 5, ucs->xAxisDirection.x);
        cmd.bind( 6, ucs->xAxisDirection.y);
        cmd.bind( 7, ucs->xAxisDirection.z);
        cmd.bind( 8, ucs->yAxisDirection.x);
        cmd.bind( 9, ucs->yAxisDirection.y);
        cmd.bind(10, ucs->yAxisDirection.z);

        cmd.executeNonQuery();
    }

    // update existing UCS:
    else {
        RS_DbCommand cmd(
            db, 
            "UPDATE Ucs "
            "SET originX=?, originY=?, originZ=?, "
            "    xAxisDirectionX=?, xAxisDirectionY=?, xAxisDirectionZ=?, "
            "    yAxisDirectionX=?, yAxisDirectionY=?, yAxisDirectionZ=? "
            "WHERE id=?"
        );
        
        cmd.bind( 1, ucs->origin.x);
        cmd.bind( 2, ucs->origin.y);
        cmd.bind( 3, ucs->origin.z);
        cmd.bind( 4, ucs->xAxisDirection.x);
        cmd.bind( 5, ucs->xAxisDirection.y);
        cmd.bind( 6, ucs->xAxisDirection.z);
        cmd.bind( 7, ucs->yAxisDirection.x);
        cmd.bind( 8, ucs->yAxisDirection.y);
        cmd.bind( 9, ucs->yAxisDirection.z);
        cmd.bind(10, ucs->getId());

        cmd.executeNonQuery();
    }
}



void RS_DbsUcsType::deleteObject(RS_DbConnection& db, RS_Object::Id objectId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Ucs "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();

    RS_DbsObjectType::deleteObject(db, objectId);
}
