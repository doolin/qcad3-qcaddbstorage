#include "RS_DbsLine"
#include "RS_DbClient"
#include "RS_Line"
#include "RS_DbsEntityRegistry"



void RS_DbsLine::registerType() {
    RS_DbsEntityRegistry::registerEntityType(RS_Line::getTypeIdStatic(), new RS_DbsLine());
}



void RS_DbsLine::initDb(RS_DbConnection& db) {
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



RS_Entity* RS_DbsLine::instantiate(RS_DbConnection& db, RS_Entity::Id entityId) {
    RS_LineData data;
    readEntityData(db, data, entityId);
    return new RS_Line(data, entityId);
}



void RS_DbsLine::readEntityData(RS_DbConnection& db, RS_LineData& data, RS_Entity::Id entityId) {
    RS_DbsEntity::readEntityData(db, data, entityId);

    RS_DbCommand cmd(
        db, 
        "SELECT x1,y1,z1,x2,y2,z2 "
        "FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);

	RS_DbReader reader = cmd.executeReader();
	if (!reader.read()) {
        RS_Debug::error("RS_DbsLine::readEntityData: "
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



void RS_DbsLine::save(RS_DbConnection& db, RS_Entity& entity) {
    RS_Line& line = dynamic_cast<RS_Line&>(entity);

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



void RS_DbsLine::deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Line "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    cmd.executeNonQuery();
}

