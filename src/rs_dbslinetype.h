#include "RS_DbsEntityType"
#include "RS_LineEntity"



/**
 * Handles the DB storage for line entities.
 *
 * Line entities are stored in a table with the following schema:
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
class RS_DbsLineType : public RS_DbsEntityType {
public:
    RS_DbsLineType() : RS_DbsEntityType() {}
    virtual ~RS_DbsLineType() {}
    
    static void registerType();

    virtual void initDb(RS_DbConnection& db);
    virtual RS_Object* loadObject(RS_DbConnection& db, RS_Object::Id objectId);
    virtual void loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId);
    virtual void saveObject(RS_DbConnection& db, RS_Object& entity, bool isNew);
    virtual void deleteObject(RS_DbConnection& db, RS_Object::Id objectId);
};
