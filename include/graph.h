#ifndef GRAPH_H_
#define GRAPH_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace lumin {
class View;

class IGraph {
 public:
  virtual ~IGraph() { }
 public:
  virtual void add_view(const std::shared_ptr<View>& view) = 0;
  virtual std::vector<std::string> apps() const = 0;
  virtual std::vector<std::shared_ptr<View>> mapped_views() const = 0;
  virtual std::vector<std::shared_ptr<View>> flattened_views() const = 0;
  virtual void bring_to_front(View *view) = 0;
  virtual void remove_deleted_views() = 0;
};

struct GraphNode
{
  void remove_deleted_views();
  bool has_view(View* view) const;
  void bring_to_front(View *view);

  std::string id;
  std::vector<std::shared_ptr<View>> views;
};

class Graph : public IGraph {
 public:
  void add_view(const std::shared_ptr<View>& view);
  void bring_to_front(View *view);
  void remove_deleted_views();

  std::vector<std::string> apps() const;
  std::vector<std::shared_ptr<View>> flattened_views() const;
  std::vector<std::shared_ptr<View>> mapped_views() const;

 private:
  std::vector<std::shared_ptr<GraphNode>> nodes_;
};

}  // namespace lumin

#endif  // GRAPH_H_
