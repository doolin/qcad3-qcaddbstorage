#include "RS_DbsEntityTypeRegistry"

#include <cassert>

#include "RS_DbsLineType"
#include "RS_Debug"
#include "RS_Entity"
#include "RS_Line"

#include "RS_DbClient"
    

    
std::map<RS_Entity::EntityTypeId, RS_DbsEntityType*> RS_DbsEntityTypeRegistry::dbEntities;



/**
 * Registers all entity types that are part of the qcadcode module.
 */
void RS_DbsEntityTypeRegistry::registerStandardEntityTypes() {
    RS_DbsLineType::registerType();
}



/**
 * Registers an entity type from a unique type ID, a initialization
 * function that initializes the DB and a factory function that produces 
 * entities of that type.
 *
 * @param initDbFunction: Pointer to a static member function of the
 *      entity class which initializes the table(s) necessary for 
 *      storing entities of this type.
 * @param factoryFunction: Pointer to a static member function of the
 *      entity class which creates an entity by a given ID.
 */
void RS_DbsEntityTypeRegistry::registerEntityType(
    RS_Entity::EntityTypeId typeId, 
    RS_DbsEntityType* dbEntity) {

    if (dbEntities.count(typeId)==0) {
        dbEntities[typeId] = dbEntity;
    }
    else {
        RS_Debug::error("RS_DbsEntityTypeRegistry::registerEntityType: "
            "duplicate type ID: %d", typeId);
    }
}



/**
 * Initializes the DB for all registered entity types. All entity types
 * must be registered before calling this function.
 */
void RS_DbsEntityTypeRegistry::initDb(RS_DbConnection& db) {
    std::map<RS_Entity::EntityTypeId, RS_DbsEntityType*>::iterator it;
    for (it=dbEntities.begin(); it!=dbEntities.end(); it++) {
        it->second->initDb(db);
    }
}



/**
 * \return The factory function that can be used to produce entities of
 *      the given type or NULL.
 */
RS_DbsEntityType* RS_DbsEntityTypeRegistry::getDbEntity(RS_Entity::EntityTypeId typeId) {
    if (dbEntities.count(typeId)==1) {
        return dbEntities[typeId];
    }
    else {
        return NULL;
    }
}
    
    
    
void RS_DbsEntityTypeRegistry::deleteEntity(RS_DbConnection& db, RS_Entity::EntityTypeId typeId, RS_Entity::Id entityId) {
    dbEntities[typeId]->deleteEntity(db, entityId);
}


/**
 * Cleans up all known entity types. Call this at the end of an application,
 * just before the application is terminated.
 */
void RS_DbsEntityTypeRegistry::cleanUp() {
    std::map<RS_Entity::EntityTypeId, RS_DbsEntityType*>::iterator it;
    for (it=dbEntities.begin(); it!=dbEntities.end(); it++) {
        delete (it->second);
    }
    dbEntities.clear();
}
