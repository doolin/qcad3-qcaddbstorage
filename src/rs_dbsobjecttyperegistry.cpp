#include "RS_DbsObjectTypeRegistry"

#include <cassert>

#include "RS_DbsLineType"
#include "RS_DbsUcsType"
#include "RS_Debug"
#include "RS_Object"
#include "RS_Line"

#include "RS_DbClient"
    

    
std::map<RS_Object::ObjectTypeId, RS_DbsObjectType*> RS_DbsObjectTypeRegistry::dbObjects;



/**
 * Registers all entity types that are part of the qcadcode module.
 */
void RS_DbsObjectTypeRegistry::registerStandardObjectTypes() {
    RS_DbsUcsType::registerType();
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
void RS_DbsObjectTypeRegistry::registerObjectType(
    RS_Object::ObjectTypeId typeId, 
    RS_DbsObjectType* dbObject) {

    if (dbObjects.count(typeId)==0) {
        dbObjects[typeId] = dbObject;
    }
    else {
        RS_Debug::error("RS_DbsObjectTypeRegistry::registerObjectType: "
            "duplicate type ID: %d", typeId);
    }
}



/**
 * Initializes the DB for all registered entity types. All entity types
 * must be registered before calling this function.
 */
void RS_DbsObjectTypeRegistry::initDb(RS_DbConnection& db) {
    std::map<RS_Object::ObjectTypeId, RS_DbsObjectType*>::iterator it;
    for (it=dbObjects.begin(); it!=dbObjects.end(); it++) {
        it->second->initDb(db);
    }
}



/**
 * \return The factory function that can be used to produce entities of
 *      the given type or NULL.
 */
RS_DbsObjectType* RS_DbsObjectTypeRegistry::getDbObject(RS_Object::ObjectTypeId typeId) {
    if (dbObjects.count(typeId)==1) {
        return dbObjects[typeId];
    }
    else {
        return NULL;
    }
}
    
    
    
/*
void RS_DbsObjectTypeRegistry::deleteObject(RS_DbConnection& db, RS_Object::ObjectTypeId typeId, RS_Object::Id entityId) {
    dbObjects[typeId]->deleteObject(db, entityId);
}
*/


/**
 * Cleans up all known entity types. Call this at the end of an application,
 * just before the application is terminated.
 */
void RS_DbsObjectTypeRegistry::cleanUp() {
    std::map<RS_Object::ObjectTypeId, RS_DbsObjectType*>::iterator it;
    for (it=dbObjects.begin(); it!=dbObjects.end(); it++) {
        delete (it->second);
    }
    dbObjects.clear();
}
