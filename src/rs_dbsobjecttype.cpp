#include "RS_DbsObjectType"
#include "RS_DbConnection"
#include "RS_DbCommand"



/**
 * Initializes the DB for this object type (creating tables, etc.).
 * The implementation of the base class must also be called.
 */
void RS_DbsObjectType::initDb(RS_DbConnection& db) {
    db.executeNonQuery(
        "CREATE TABLE IF NOT EXISTS Object("
            "id INTEGER PRIMARY KEY, "
            "objectTypeId INTEGER, "
            "undoStatus INTEGER"
        ");"
    );
}



/**
 * Loads the object data for the object with the given object id into the 
 * given object.
 */
void RS_DbsObjectType::loadObject(RS_DbConnection& /*db*/, RS_Object& object, RS_Object::Id objectId) {
    // nothing to load at this level.
    object.setId(objectId);
}



/**
 * Saves the given object to the DB.
 * The given object must be of the correct type, otherwise results are
 * undefined.
 * The implementation of the base class must also be called.
 */
void RS_DbsObjectType::saveObject(RS_DbConnection& db, RS_Object& object, bool isNew) {
    // new object:
    if (isNew) {
        // generic object information has to be stored for all object types:
        RS_DbCommand cmd(
            db, 
            "INSERT INTO Object VALUES(?,?,?);"
        );

        cmd.bind(1);
        cmd.bind(2, object.getObjectTypeId());
        cmd.bind(3, 0);

        cmd.executeNonQuery();
        object.setId(db.getLastInsertedRowId());
    }

    // existing object:
    else {
        // nothing to update at this level.
    }
    
}



/**
 * Deletes the object with the given ID.
 * The implementation of the base class must also be called.
 */
void RS_DbsObjectType::deleteObject(RS_DbConnection& db, RS_Object::Id objectId) {
    RS_DbCommand cmd(
        db, 
        "DELETE FROM Object "
        "WHERE id=?"
    );
    cmd.bind(1, objectId);
    cmd.executeNonQuery();
}
