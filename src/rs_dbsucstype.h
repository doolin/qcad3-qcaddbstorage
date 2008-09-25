#include "RS_DbsObjectType"
#include "RS_Ucs"



/**
 * Handles the DB storage for UCS.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsUcsType : public RS_DbsObjectType {
public:
    RS_DbsUcsType() : RS_DbsObjectType() {}
    virtual ~RS_DbsUcsType() {}
    
    static void registerType();

    virtual void initDb(RS_DbConnection& db);
    virtual RS_Object* loadObject(RS_DbConnection& db, RS_Object::Id objectId);
    virtual void loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId);
    virtual void saveObject(RS_DbConnection& db, RS_Object& entity, bool isNew);
    virtual void deleteObject(RS_DbConnection& db, RS_Object::Id objectId);
};
