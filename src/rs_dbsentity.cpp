#include "RS_DbsEntity"
#include "RS_DbCommand"
#include "RS_DbReader"



void RS_DbsEntity::readEntityData(RS_DbConnection& db, RS_EntityData& data, RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT selectionStatus "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);

	RS_DbReader reader = cmd.executeReader();
	if (!reader.read()) {
        RS_Debug::error("RS_DbsEntity::readEntityData: "
            "cannot read data for entity %d", entityId);
        return;
    }

    data.selectionStatus = reader.getInt(0);
    RS_Debug::debug("RS_DbsEntity::readEntityData: %d", data.selectionStatus);
}
