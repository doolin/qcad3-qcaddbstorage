#include "RS_DbsEntity"
#include "RS_LineEntity"



/**
 * Handles the DB storage for line entities.
 *
 * Line entities are stored in a table with the following schema:
 *
 * The DB uses the following tables to store documents:
 *
 * \b Line
 * - \b id: Entity ID.
 * - \b x1: X ordiante of the start point.
 * - \b y1: Y ordiante of the start point.
 * - \b z1: Z ordiante of the start point.
 * - \b x2: X ordiante of the end point.
 * - \b y2: Y ordiante of the end point.
 * - \b z2: Z ordiante of the end point.
 *
 * The \b Line table stores data that is specific to line entities.
 * Common data for all entities is stored in table \b Entity.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsLine : public RS_DbsEntity {
public:
    RS_DbsLine() : RS_DbsEntity() {}
    virtual ~RS_DbsLine() {}
    
    static void registerType();

    virtual void initDb(RS_DbConnection& db);
    virtual RS_Entity* instantiate(RS_DbConnection& db, RS_Entity::Id entityId);
    virtual void readEntityData(RS_DbConnection& db, RS_LineData& data, RS_Entity::Id entityId);
    virtual void save(RS_DbConnection& db, RS_Entity& entity);
    virtual void deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId);
};
