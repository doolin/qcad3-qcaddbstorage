#ifndef RS_DBOBJECTTYPE_H
#define RS_DBOBJECTTYPE_H

#include "RS_Object"
#include "RS_DbsObjectTypeRegistry"

class RS_DbConnection;



/**
 * This interface must be implemented by classes that handle the 
 * DB storage for an object type. The purpose of such classes
 * is to separate storage from the object implementation.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsObjectType {
public:
    RS_DbsObjectType() {}
    virtual ~RS_DbsObjectType() {}

    /**
     * Initializes the DB for this object type (creating tables, etc.).
     */
    virtual void initDb(RS_DbConnection& db);
    
    /**
     * Instantiates the object with the given \c objectId from the DB.
     * The caller is responsible for deleting the instance.
     */
    virtual RS_Object* loadObject(RS_DbConnection& db, RS_Object::Id objectId) = 0;
    
    /**
     * Loads the object data for the object with the given object id into the 
     * given object.
     */
    virtual void loadObject(RS_DbConnection& db, RS_Object& object, RS_Object::Id objectId);

    /**
     * Saves the given object to the DB.
     * The given object must be of the correct type, otherwise results are
     * undefined.
     */
    virtual void saveObject(RS_DbConnection& db, RS_Object& object, bool isNew);
    
    /**
     * Deletes the object with the given ID.
     */
    virtual void deleteObject(RS_DbConnection& db, RS_Object::Id objectId);

    //virtual void readEntityData(RS_DbConnection& db, RS_EntityData& data, RS_Entity::Id entityId);
};

#endif
