#ifndef CreateControlNetWorkOrder_H
#define CreateControlNetWorkOrder_H

#include "WorkOrder.h"

namespace Isis {
  class Control;
  class Project;

  /**
   * @brief Create a new empty control network in the project
   *
   * This work order creates a new, empty control network and adds it to the project.
   * Users can then add control points to it using the control network editor.
   *
   * @author 2026-06-04 Amy Stamile
   */
  class CreateControlNetWorkOrder : public WorkOrder {
      Q_OBJECT
    public:
      CreateControlNetWorkOrder(Project *project);
      CreateControlNetWorkOrder(const CreateControlNetWorkOrder &other);
      ~CreateControlNetWorkOrder();

      virtual CreateControlNetWorkOrder *clone() const;

      virtual bool isExecutable(ProjectItem *item);
      bool setupExecution();
      void execute();

    protected:
      void postExecution();

    private:
      CreateControlNetWorkOrder &operator=(const CreateControlNetWorkOrder &rhs);

    private:
      Control *m_newControl; //!< The newly created control
  };
}
#endif
