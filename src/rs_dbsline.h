#include "RS_DbsEntity"



/**
 * Handles the DB storage for line entities.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsLine : public RS_DbsEntity {
private:
    /**
     * Private constructor prevents from instantiation other than through
     * \ref registerType.
     */
    RS_DbsLine() {}

public:
    virtual ~RS_DbsLine() {}

    static void registerType();

    virtual void initDb(RS_DbConnection& db);
    virtual RS_Entity* instantiate(RS_DbConnection& db, RS_Entity::Id entityId);
    virtual void save(RS_DbConnection& db, RS_Entity& entity);
    virtual void deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId);
};
