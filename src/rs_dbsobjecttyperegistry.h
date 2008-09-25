#ifndef RS_DBSOBJECTREGISTRY_H
#define RS_DBSOBJECTREGISTRY_H

#include <map>

#include "RS_Object"
#include "RS_Debug"

class RS_DbConnection;
class RS_DbsObjectType;



/**
 * Registry of all available object types. If your library or
 * application adds custom object types, a DB storage class needs 
 * to be registered for them. For example:
 * 
 * \code
 * RS_DbsObjectTypeRegistry::registerObjectType(100, new RS_DbsMyObjectType());
 * \endcode
 *
 * Where \c RS_DbsMyObjectType implements RS_DbsObjectType.
 * 
 * This registration is usually done from within a static method 
 * inside the custom entity class, for example:
 *
 * \code
 * void main() {
 *     RS_DbsMyObjectType::registerType();
 * }
 *
 * void RS_DbsMyObjectType::registerType() {
 *     RS_DbsObjectTypeRegistry::registerObjectType(new RS_DbsMyObjectType());
 * }
 * \endcode
 *
 * \author Andrew Mustun
 * \ingroup qcadcore
 */
class RS_DbsObjectTypeRegistry {
public:
    static void registerStandardObjectTypes();
    static void cleanUp();
    
    static void registerObjectType(
        RS_Object::ObjectTypeId typeId, 
        RS_DbsObjectType* dbObject
    );

    static void initDb(RS_DbConnection& db);

    static RS_DbsObjectType* getDbObject(RS_Object::ObjectTypeId objectTypeId);

    //static void deleteObject(RS_DbConnection& db, RS_Object::ObjectTypeId typeId, RS_Object::Id entityId);

private:
    static std::map<RS_Object::ObjectTypeId, RS_DbsObjectType*> dbObjects;
    //static RS_Object::ObjectTypeId objectTypeIdCounter;
};

#endif
