#ifndef RS_DBENTITY_H
#define RS_DBENTITY_H

#include "RS_Entity"
#include "RS_DbsObjectType"
#include "RS_DbsObjectTypeRegistry"

class RS_DbConnection;



/**
 * This interface must be implemented by classes that handle the 
 * DB storage for an entity type. The purpose of such classes
 * is to separate storage from the entity implementation.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsEntityType : public RS_DbsObjectType {
public:
    RS_DbsEntityType() {}
    virtual ~RS_DbsEntityType() {}

    virtual void initDb(RS_DbConnection& db);
    virtual void loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId);
    virtual void saveObject(RS_DbConnection& db, RS_Object& object, bool isNew);
    virtual void deleteObject(RS_DbConnection& db, RS_Object::Id objectId);
};

#endif
