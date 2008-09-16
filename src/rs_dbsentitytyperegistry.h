#ifndef RS_DBENTITYREGISTRY_H
#define RS_DBENTITYREGISTRY_H

#include <map>

#include "RS_Entity"
#include "RS_Debug"

class RS_DbConnection;
class RS_DbsEntityType;



/**
 * Registry of all available entity types. If your library or
 * application adds custom entity types, a DB storage class needs 
 * to be registered for them. For example:
 * 
 * \code
 * RS_EntityRegistry::registerEntityType(100, new RS_DbsMyEntityType()); 
 * \endcode
 *
 * Where \c RS_DbsMyEntityType implements RS_DbsEntityType.
 * 
 * This registration is usually done from within a static method 
 * inside the custom entity class, for example:
 *
 * \code
 * void main() {
 *     RS_DbsMyEntityType::registerType();
 * }
 *
 * void RS_DbsMyEntityType::registerType() {
 *     RS_DbsEntityTypeRegistry::registerEntityType(new RS_DbsMyEntityType());
 * }
 * \endcode
 *
 * \author Andrew Mustun
 * \ingroup qcadcore
 */
class RS_DbsEntityTypeRegistry {
public:
    static void registerStandardEntityTypes();
    static void cleanUp();
    
    static void registerEntityType(
        RS_Entity::EntityTypeId typeId, 
        RS_DbsEntityType* dbEntity
    );

    static void initDb(RS_DbConnection& db);

    static RS_DbsEntityType* getDbEntity(RS_Entity::EntityTypeId typeId);

    static void deleteEntity(RS_DbConnection& db, RS_Entity::EntityTypeId typeId, RS_Entity::Id entityId);

private:
    static std::map<RS_Entity::EntityTypeId, RS_DbsEntityType*> dbEntities;
    static RS_Entity::EntityTypeId typeIdCounter;
};

#endif
