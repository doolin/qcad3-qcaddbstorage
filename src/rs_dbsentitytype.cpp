#include "RS_DbsEntityType"
#include "RS_DbCommand"
#include "RS_DbReader"



void RS_DbsEntityType::readEntityData(RS_DbConnection& db, RS_EntityData& data, RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT selectionStatus "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);

    RS_DbReader reader = cmd.executeReader();
    if (!reader.read()) {
        RS_Debug::error("RS_DbsEntityType::readEntityData: "
            "cannot read data for entity %d", entityId);
        return;
    }

    data.selectionStatus = (reader.getInt(0)!=0);
    RS_Debug::debug("RS_DbsEntityType::readEntityData: %d", data.selectionStatus);
}
