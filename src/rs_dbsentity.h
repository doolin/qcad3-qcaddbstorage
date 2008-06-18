#ifndef RS_DBENTITY_H
#define RS_DBENTITY_H

#include "RS_Entity"

class RS_DbConnection;



/**
 * This interface must be implemented by classes that handle the 
 * DB storage for an entity class. The purpose of such classes
 * is to separate storage from the entity implementation.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsEntity {
public:
    virtual ~RS_DbsEntity() {}

    /**
     * This static function registers this entity type with 
     * the \ref RS_DbsEntityRegistry. For every custom entity
     * class the application has to call this function once in 
     * the beginning to make the custom entity type available
     * to the application.
     *
     * E.g. main() might call MyEntity::registerType(); to 
     * make MyEntity available as a valid entity type.
     */
    static void registerType();

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
    virtual void save(RS_DbConnection& db, RS_Entity& entity) = 0;
    
    virtual void deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId) = 0;
};

#endif
