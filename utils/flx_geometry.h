#ifndef flx_GEOMETRY_H
#define flx_GEOMETRY_H

// interface for geometry
class flx_geometry {
public:
  virtual float get_center_x()=0;
  virtual float get_center_y()=0;
  virtual float get_bb_x()=0;
  virtual float get_bb_y()=0;
  virtual float get_bb_width()=0;
  virtual float get_bb_height() =0;
};

class flx_point : public flx_geometry {
private:
  float x;
  float y;
public:
  flx_point(float x, float y) : x(x), y(y) {}
  float get_center_x() override {
    return x;
  }
  float get_center_y() override {
    return y;
  }
  float get_bb_x() override {
    return x;
  }
  float get_bb_y() override {
    return y;
  }
  float get_bb_width() override {
    return 0;
  }
  float get_bb_height() override {
    return 0;
  }
};

#endif // flx_GEOMETRY_H
