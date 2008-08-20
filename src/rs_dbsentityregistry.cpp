#include "RS_DbsEntityRegistry"

#include <cassert>

#include "RS_DbsLine"
#include "RS_Debug"
#include "RS_Entity"
#include "RS_Line"

#include "RS_DbClient"
    

    
std::map<RS_Entity::TypeId, RS_DbsEntity*> RS_DbsEntityRegistry::dbEntities;



/**
 * Registers all entity types that are part of the qcadcode module.
 */
void RS_DbsEntityRegistry::registerStandardEntityTypes() {
    RS_DbsLine::registerType();
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
void RS_DbsEntityRegistry::registerEntityType(
    RS_Entity::TypeId typeId, 
    RS_DbsEntity* dbEntity) {

    if (dbEntities.count(typeId)==0) {
        dbEntities[typeId] = dbEntity;
    }
    else {
        RS_Debug::error("RS_DbsEntityRegistry::registerEntityType: "
            "duplicate type ID: %d", typeId);
    }
}



/**
 * Initializes the DB for all registered entity types. All entity types
 * must be registered before calling this function.
 */
void RS_DbsEntityRegistry::initDb(RS_DbConnection& db) {
    std::map<RS_Entity::TypeId, RS_DbsEntity*>::iterator it;
    for (it=dbEntities.begin(); it!=dbEntities.end(); it++) {
        it->second->initDb(db);
    }
}



/**
 * \return The factory function that can be used to produce entities of
 *      the given type or NULL.
 */
RS_DbsEntity* RS_DbsEntityRegistry::getDbEntity(RS_Entity::TypeId typeId) {
    if (dbEntities.count(typeId)==1) {
        return dbEntities[typeId];
    }
    else {
        return NULL;
    }
}
    
    
    
void RS_DbsEntityRegistry::deleteEntity(RS_DbConnection& db, RS_Entity::TypeId typeId, RS_Entity::Id entityId) {
    dbEntities[typeId]->deleteEntity(db, entityId);
}


/**
 * Cleans up all known entity types. Call this at the end of an application,
 * just before the application is terminated.
 */
void RS_DbsEntityRegistry::cleanUp() {
    std::map<RS_Entity::TypeId, RS_DbsEntity*>::iterator it;
    for (it=dbEntities.begin(); it!=dbEntities.end(); it++) {
        delete (it->second);
    }
    dbEntities.clear();
}
