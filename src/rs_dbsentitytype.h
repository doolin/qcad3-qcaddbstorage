#ifndef RS_DBENTITY_H
#define RS_DBENTITY_H

#include "RS_Entity"
#include "RS_DbsEntityTypeRegistry"

class RS_DbConnection;



/**
 * This interface must be implemented by classes that handle the 
 * DB storage for an entity type. The purpose of such classes
 * is to separate storage from the entity implementation.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsEntityType {
public:
    RS_DbsEntityType() {}
    virtual ~RS_DbsEntityType() {}

    /**
     * Initializes the DB for this entity type (creating tables, etc.).
     */
    virtual void initDb(RS_DbConnection& db) = 0;
    
    /**
     * Instantiates the entity with the given \c entityId from the DB.
     * The caller is responsible for deleting the instance.
     */
    virtual RS_Entity* instantiate(RS_DbConnection& db, RS_Entity::Id entityId) = 0;

    /**
     * Saves the given entity to the DB.
     * The given entity must be of the correct type, otherwise results are
     * undefined.
     */
    virtual void save(RS_DbConnection& db, RS_Entity& entity, bool isNew) = 0;
    
    virtual void deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId) = 0;

    virtual void readEntityData(RS_DbConnection& db, RS_EntityData& data, RS_Entity::Id entityId);
};

#endif
