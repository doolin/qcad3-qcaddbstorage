#ifndef RS_DBENTITYREGISTRY_H
#define RS_DBENTITYREGISTRY_H

#include <map>

#include "RS_Entity"
#include "RS_Debug"

class RS_DbConnection;
class RS_DbsEntity;



/**
 * Registry of all available entity types. If your library or
 * application adds custom entity types they need to be registered
 * with this class for example with:
 * 
 * \code
 * RS_EntityRegistry::registerEntityType(100, RS_MyEntity::initDb, RS_MyEntity::factory); 
 * \endcode
 * 
 * This registration is usually done from within a static method 
 * inside the custom entity class. initDb() is a static function that
 * initializes the DB for storing entities of the custom type.
 * factory() is a function that creates entities of that type.
 *
 * \author Andrew Mustun
 * \ingroup qcadcore
 */
class RS_DbsEntityRegistry {
public:
    static void registerStandardEntityTypes();
    
    static void registerEntityType(
        RS_Entity::TypeId typeId, 
        RS_DbsEntity* dbEntity
    );

    static void initDb(RS_DbConnection& db);

    static RS_DbsEntity* getDbEntity(RS_Entity::TypeId typeId);

    static void deleteEntity(RS_DbConnection& db, RS_Entity::TypeId typeId, RS_Entity::Id entityId);

private:
    static std::map<RS_Entity::TypeId, RS_DbsEntity*> dbEntities;
    static RS_Entity::TypeId typeIdCounter;
};

#endif
