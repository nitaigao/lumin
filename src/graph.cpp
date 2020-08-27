#include "graph.h"

#include "view.h"

using std::find_if;

namespace lumin {

void Graph::add_view(const std::shared_ptr<View>& view)
{
  auto result = find_if(nodes_.begin(), nodes_.end(),
    [view](auto& node) { return node->id.compare(view->id()) == 0; });

  if (result != nodes_.end()) {
    auto node = *result;
    node->views.push_back(view);
  } else {
    auto node = std::make_shared<GraphNode>();
    node->id = view->id();
    node->views.push_back(view);
    nodes_.push_back(node);
  }
}

std::vector<std::string> Graph::apps() const
{
  std::vector<std::string> apps;

  for (auto& node : nodes_) {
    apps.push_back(node->id);
  }

  return apps;
}

std::vector<std::shared_ptr<View>> Graph::mapped_views() const
{
  auto views = flattened_views();
  std::vector<std::shared_ptr<View>> mapped_views;

  std::copy_if(views.begin(), views.end(), std::back_inserter(mapped_views),
    [](auto &view) { return !view->minimized && !view->deleted && view->mapped; });

  return mapped_views;
}

std::vector<std::shared_ptr<View>> Graph::flattened_views() const
{
  std::vector<std::shared_ptr<View>> flattened_views;

  for (auto& node : nodes_) {
    for (auto& view : node->views) {
      flattened_views.push_back(view);
    }
  }

  return flattened_views;
}

void Graph::bring_to_front(View *view)
{
  auto result = std::find_if(nodes_.begin(), nodes_.end(),
    [view](const auto& node) { return node->has_view(view); });
  if (result != nodes_.end()) {
    auto resultValue = *result;
    nodes_.erase(result);
    nodes_.insert(nodes_.begin(), resultValue);
    resultValue->bring_to_front(view);
  }
}

void GraphNode::bring_to_front(View *view)
{
  auto result = std::find_if(views.begin(), views.end(),
    [view](const auto& el) { return el.get() == view; });
  if (result != views.end()) {
    auto resultValue = *result;
    views.erase(result);
    views.insert(views.begin(), resultValue);
  }
}

void Graph::remove_deleted_views()
{
  for (auto& node : nodes_) {
    node->remove_deleted_views();
  }
}

bool GraphNode::has_view(View* view) const
{
  auto result = std::find_if(views.begin(), views.end(),
    [view](const auto& el) { return el.get() == view; });
  return result != views.end();
}

void GraphNode::remove_deleted_views()
{
  std::erase_if(views, [](const auto &view) { return view->deleted; });
}

}  // namespace lumin
