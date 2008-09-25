#ifndef RS_DBSTORAGE_H
#define RS_DBSTORAGE_H

#include <sstream>
#include <set>

#include "RS_Transaction"
#include "RS_AbstractStorage"
#include "RS_DbClient"



/**
 * A DB storage object implements the storage interface to store 
 * document data in an SQLite DB.
 *
 * The DB uses the following tables to store documents:
 *
 * \b Entity
 * - \b id: Entity ID.
 * - \b type: Entity type.
 * - \b blockName: Name of the block this entity is a part of or NULL
 *          for top level entities.
 * - \b layerName: Name of the layer this entity is on.
 * - \b undoStatus: 1 for entities that are undone (and therefore invisible)
 *          NULL for normal entities.
 *
 * The \b Entity table stores data that is common to all entity types.
 * The type specific data is stored in a different table, for example the
 * data that is specific to line entities is stored in table \b Line.
 *
 * \b Transaction
 * - \b id: Log entry ID.
 * - \b text: Description of transaction.
 *
 * The \b Transaction table is used for the undo/redo mechanism.
 *
 * \b TransactionEntity
 * - \b tid: Transaction log entry ID.
 * - \b eid: Entity ID.
 * 
 * The \b TransactionEntity table links every transaction log entry with
 * a number of entities that are affected by that transaction.
 *
 * \b LastTransaction
 * - \b lastTransaction: ID of the last transaction log entry.
 *
 * The \b LastTransaction table stores one value which is the ID of the
 * last transaction. This pointer wanders up and down the transaction log
 * if the user hits undo / redo.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbStorage : public RS_AbstractStorage {
public:
    enum ObjectType {
        objectTypeEntity = 0,
        objectTypeLayer = 1,
        objectTypeUcs = 2
    };

public:
    RS_DbStorage(const std::string& fileName = ":memory:");
    virtual ~RS_DbStorage();

    virtual std::set<RS_Object::Id> queryAllObjects();
    virtual std::set<RS_Entity::Id> queryAllEntities();
    virtual std::set<RS_Entity::Id> querySelectedEntities();

    virtual RS_Object* queryObject(RS_Object::Id objectId);
    virtual RS_Entity* queryEntity(RS_Entity::Id entityId);
    virtual RS_Ucs* queryUcs(RS_Ucs::Id ucsId);

    virtual void clearEntitySelection(
        std::set<RS_Entity::Id>* affectedEntities=NULL
    );

    virtual void selectEntity(
        RS_Entity::Id entityId, 
        bool add=false, 
        std::set<RS_Entity::Id>* affectedEntities=NULL
    );
    virtual void selectEntities(
        std::set<RS_Entity::Id>& entityIds, 
        bool add=false, 
        std::set<RS_Entity::Id>* affectedEntities=NULL
    );

    virtual RS_Box getBoundingBox();
    
    virtual void saveObject(RS_Object& object);
    virtual void deleteObject(RS_Object::Id objectId);

    virtual void beginTransaction();
    virtual void commitTransaction();
    
    virtual int getLastTransactionId();
    virtual void setLastTransactionId(int transactionId);
    virtual void saveTransaction(RS_Transaction& transaction);;
    virtual void deleteTransactionsFrom(int transactionId);
    virtual RS_Transaction getTransaction(int transactionId);
    virtual int getMaxTransactionId();

    virtual void toggleUndoStatus(std::set<RS_Object::Id>& objectIds);
    virtual void toggleUndoStatus(RS_Object::Id objectId);
    virtual bool getUndoStatus(RS_Object::Id objectId);

protected:
    //virtual void saveEntity(RS_Entity& entity, bool isNew);
    //virtual void deleteEntity(RS_Entity::Id entityId);
    
    //virtual void saveUcs(RS_Ucs& ucs, bool isNew);
    //virtual void deleteUcs(RS_Ucs::Id ucsId);
    
    RS_Object::ObjectTypeId getObjectTypeId(RS_Object::Id objectId);
    //RS_Entity::EntityTypeId getEntityTypeId(RS_Entity::Id entityId);
    RS_Object* queryObject(RS_Object::Id objectId, RS_Object::ObjectTypeId objectTypeId);

    static std::string getSqlList(std::set<RS_Object::Id>& values);

private:
    //! connection to SQLite DB:
    RS_DbConnection db;
};

#endif
