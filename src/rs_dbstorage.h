#ifndef RS_DBSTORAGE_H
#define RS_DBSTORAGE_H

#include <sstream>
#include <set>

#include "RS_PassiveTransaction"
#include "RS_Storage"
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
 * \b TransactionLog
 * - \b id: Log entry ID.
 * - \b text: Description of transaction.
 *
 * The \b TransactionLog table is used for the undo/redo mechanism.
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
class RS_DbStorage : public RS_Storage {
public:
    RS_DbStorage();
    virtual ~RS_DbStorage();

    virtual std::set<RS_Entity::Id> queryAll();
    virtual RS_Entity* queryEntity(RS_Entity::Id entityId);

    virtual void clearSelection(
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
    
    virtual void save(RS_Entity& entity);
    virtual void deleteEntity(RS_Entity::Id entityId);
    
    virtual void beginTransaction();
    virtual void commitTransaction();
    
    virtual int getLastTransactionId();
    virtual void setLastTransactionId(int transactionId);
    virtual void saveTransaction(RS_ActiveTransaction& transaction);;
    virtual void deleteTransactionsFrom(int transactionId);
    virtual RS_PassiveTransaction getTransaction(int transactionId);
    virtual int getMaxTransactionId();

    virtual void toggleUndoStatus(std::set<RS_Entity::Id>& entities);

protected:
    RS_Entity* queryEntity(RS_Entity::Id entityId, RS_Entity::TypeId typeId);
    RS_Entity::TypeId getEntityType(RS_Entity::Id entityId);

    std::string getSqlList(std::set<RS_Entity::Id>& values) {
        std::stringstream ss;
        ss << "(";
        std::set<RS_Entity::Id>::iterator it;
        for (it=values.begin(); it!=values.end(); ++it) {
            if (it!=values.begin()) {
                ss << ",";
            }
            ss << (*it);
        }
        ss << ")";
        return ss.str();
    }

private:
    //! connection to SQLite DB:
    RS_DbConnection db;
};

#endif
