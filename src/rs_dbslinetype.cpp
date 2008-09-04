#include "RS_DbsLineType"
#include "RS_DbClient"
#include "RS_LineEntity"
#include "RS_DbsEntityTypeRegistry"



void RS_DbsLineType::registerType() {
    RS_DbsEntityTypeRegistry::registerEntityType(RS_LineEntity::getTypeIdStatic(), new RS_DbsLineType());
}



void RS_DbsLineType::initDb(RS_DbConnection& db) {
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



RS_Entity* RS_DbsLineType::instantiate(RS_DbConnection& db, RS_Entity::Id entityId) {
    RS_LineData data;
    readEntityData(db, data, entityId);
    return new RS_LineEntity(data, entityId);
}



void RS_DbsLineType::readEntityData(RS_DbConnection& db, RS_LineData& data, RS_Entity::Id entityId) {
    RS_DbsEntityType::readEntityData(db, data, entityId);

    RS_DbCommand cmd(
        db, 
        "SELECT x1,y1,z1,x2,y2,z2 "
        "FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbsLineType::readEntityData: "
            "cannot read data for entity %d", entityId);
        return;
    }

    data.startPoint.x = reader.getDouble(0);
    data.startPoint.y = reader.getDouble(1);
    data.startPoint.z = reader.getDouble(2);
    data.endPoint.x = reader.getDouble(3);
    data.endPoint.y = reader.getDouble(4);
    data.endPoint.z = reader.getDouble(5);
}



void RS_DbsLineType::save(RS_DbConnection& db, RS_Entity& entity) {
    RS_LineEntity& line = dynamic_cast<RS_LineEntity&>(entity);

    RS_DbCommand cmd(
        db, 
        "INSERT INTO Line "
        "VALUES(?,?,?,?,?,?,?)"
    );
    RS_LineData& data = line.getData();
    cmd.bind(1, entity.getId());
    cmd.bind(2, data.startPoint.x);
    cmd.bind(3, data.startPoint.y);
    cmd.bind(4, data.startPoint.z);
    cmd.bind(5, data.endPoint.x);
    cmd.bind(6, data.endPoint.y);
    cmd.bind(7, data.endPoint.z);
    cmd.executeNonQuery();
}



void RS_DbsLineType::deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    cmd.executeNonQuery();
}
