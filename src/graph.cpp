#include "graph.h"

#include "view.h"

using std::find_if;

namespace lumin {

void Graph::add_view(const std::shared_ptr<View>& view)
{
  views_.push_back(view);
}

std::vector<std::string> Graph::apps() const
{
  std::vector<std::string> apps;

  return apps;
}

std::vector<std::shared_ptr<View>> Graph::mapped_views() const
{
  std::vector<std::shared_ptr<View>> mapped_views;

  std::copy_if(views_.begin(), views_.end(), std::back_inserter(mapped_views),
    [](auto &view) { return !view->minimized && !view->deleted && view->mapped; });

  return mapped_views;
}

void Graph::bring_to_front(View *view)
{
  auto result = std::find_if(views_.begin(), views_.end(),
    [view](const auto& v) { return v.get() == view; });
  if (result != views_.end()) {
    auto resultValue = *result;
    views_.erase(result);
    views_.insert(views_.begin(), resultValue);
  }
}

void Graph::remove_deleted_views()
{
  std::erase_if(views_, [](const auto &view) { return view->deleted; });
}

}  // namespace lumin
