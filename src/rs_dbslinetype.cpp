#include "RS_DbsLineType"
#include "RS_DbClient"
#include "RS_LineEntity"
#include "RS_DbsObjectTypeRegistry"



void RS_DbsLineType::registerType() {
    RS_DbsObjectTypeRegistry::registerObjectType(
        RS_LineEntity::getObjectTypeIdStatic(), 
        new RS_DbsLineType()
    );
}



void RS_DbsLineType::initDb(RS_DbConnection& db) {
    RS_DbsEntityType::initDb(db);

    db.executeNonQuery(
        "CREATE TABLE Line("
            "id INT PRIMARY KEY, "
            "x1 REAL, "
            "y1 REAL, "
            "z1 REAL, "
            "x2 REAL, "
            "y2 REAL, "
            "z2 REAL"
        ");"
    );
}



RS_Object* RS_DbsLineType::loadObject(RS_DbConnection& db, RS_Object::Id objectId) {
    RS_LineData data;
    RS_LineEntity* line = new RS_LineEntity(data, objectId);
    loadObject(db, *line, objectId);
    return line;
}



void RS_DbsLineType::loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId) {
    RS_DbsEntityType::loadObject(db, object, objectId);

    RS_LineEntity* line = dynamic_cast<RS_LineEntity*>(&object);
    if (line==NULL) {
        RS_Debug::error("RS_DbsLineType::loadObject: given object not a line");
        return;
    }

    RS_DbCommand cmd(
        db, 
        "SELECT x1,y1,z1,x2,y2,z2 "
        "FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbsLineType::readEntityData: "
            "cannot read data for entity %d", objectId);
        return;
    }

    RS_Vector startPoint;
    RS_Vector endPoint;

    startPoint.x = reader.getDouble(0);
    startPoint.y = reader.getDouble(1);
    startPoint.z = reader.getDouble(2);
    endPoint.x = reader.getDouble(3);
    endPoint.y = reader.getDouble(4);
    endPoint.z = reader.getDouble(5);

    line->getData().startPoint = startPoint;
    line->getData().endPoint = endPoint;
}



void RS_DbsLineType::saveObject(RS_DbConnection& db, RS_Object& object, bool isNew) {
    RS_DbsEntityType::saveObject(db, object, isNew);

    RS_LineEntity* line = dynamic_cast<RS_LineEntity*>(&object);
    if (line==NULL) {
        RS_Debug::error("RS_DbsLineType::saveObject: given object not a line");
        return;
    }

    RS_LineData& data = line->getData();

    // add line as new entity:
    if (isNew) {
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Line "
            "VALUES(?,?,?,?,?,?,?)"
        );

        cmd.bind(1, line->getId());
        cmd.bind(2, data.startPoint.x);
        cmd.bind(3, data.startPoint.y);
        cmd.bind(4, data.startPoint.z);
        cmd.bind(5, data.endPoint.x);
        cmd.bind(6, data.endPoint.y);
        cmd.bind(7, data.endPoint.z);
        
        cmd.executeNonQuery();
    }

    // update existing line:
    else {
        RS_DbCommand cmd(
            db, 
            "UPDATE Line "
            "SET x1=?, y1=?, z1=?, x2=?, y2=?, z2=? "
            "WHERE id=?"
        );
        
        cmd.bind(1, data.startPoint.x);
        cmd.bind(2, data.startPoint.y);
        cmd.bind(3, data.startPoint.z);
        cmd.bind(4, data.endPoint.x);
        cmd.bind(5, data.endPoint.y);
        cmd.bind(6, data.endPoint.z);
        cmd.bind(7, line->getId());
        
        cmd.executeNonQuery();
    }
}



void RS_DbsLineType::deleteObject(RS_DbConnection& db, RS_Object::Id objectId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();

    RS_DbsEntityType::deleteObject(db, objectId);
}

