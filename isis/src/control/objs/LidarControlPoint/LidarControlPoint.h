#ifndef LidarControlPoint_h
#define LidarControlPoint_h

/**
 * @file
 * $Revision: 1.14 $
 * $Date: 2009/09/08 17:38:17 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "iTime.h"

namespace Isis {
  /**
   * @brief A lidar control ControlPoint
   * 
   * A lidar control point that extends from ControlPoint. Currently only works for LOLA data
   * 
   * @author 2018-01-29 Makayla Shepherd
   * 
   * @see ControlPoint
   */
  
  class LidarControlPoint : public ControlPoint {
    
  public:
    
    LidarControlPoint();
    LidarControlPoint(double time, double range, double sigmaRange);
    LidarControlPoint(iTime time, double range, double sigmaRange);
    LidarControlPoint(QString time, double range, double sigmaRange);
    
    ~LidarControlPoint();
    
    void setRange(double range);
    void setSigmaRange(double sigmaRange);
    void setTime(double time);
    void setTime(iTime time);
    void setTime(QString time);
    
    double range();
    double sigmaRange();
    iTime time();
    
    ControlMeasure backProject(Cube cube);
    
  private:
    double m_range;
    double m_sigmaRange;
    iTime m_time;
    
  };
}

#endif
    
