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
 * Registers all object types that are part of the qcadcore module
 * (UCS, layer, line, ...).
 */
void RS_DbsObjectTypeRegistry::registerStandardObjectTypes() {
    RS_DbsUcsType::registerType();
    RS_DbsLineType::registerType();
}



/**
 * Registers a new (non-standard) object type with a unique type ID.
 *
 * @param dbObject Instance of an RS_DbsObjectType implementation. This
 *      object handles all DB interaction for the new object type.
 */
void RS_DbsObjectTypeRegistry::registerObjectType(
    RS_Object::ObjectTypeId objectTypeId, 
    RS_DbsObjectType* dbObject) {

    if (dbObjects.count(objectTypeId)==0) {
        dbObjects[objectTypeId] = dbObject;
    }
    else {
        RS_Debug::error("RS_DbsObjectTypeRegistry::registerObjectType: "
            "duplicate type ID: %d", objectTypeId);
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
RS_DbsObjectType* RS_DbsObjectTypeRegistry::getDbObject(RS_Object::ObjectTypeId objectTypeId) {
    if (dbObjects.count(objectTypeId)==1) {
        return dbObjects[objectTypeId];
    }
    else {
        return NULL;
    }
}
    
    
    
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
