#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <set>

#include "router.h"

class Stop {
public:
	Stop() = delete;
	Stop(string name, double latitude, double longitude, unordered_map<string, size_t> distancesToAnotherStops) :
		name_(name), latitude_(latitude), longitude_(longitude),  distancesToAnotherStops_(distancesToAnotherStops){}

	double CalcGeoLength(const Stop& other) const {
		return 6371000 * acos(sin(latitude_ * 3.1415926535 / 180) * sin(other.latitude_ * 3.1415926535 / 180)
				+ cos(latitude_ * 3.1415926535 / 180) * cos(other.latitude_ * 3.1415926535 / 180) * cos(other.longitude_ * 3.1415926535 / 180 - longitude_ * 3.1415926535 / 180));
	}
	double CalcPathLength(const Stop& other) const {
		if (distancesToAnotherStops_.count(other.GetName()) > 0){
			return distancesToAnotherStops_.at(other.GetName());
		}
		if (name_ == other.GetName()) {return 0.0;}
		return CalcGeoLength(other);
	}
	const string& GetName() const {
		return name_;
	}
	void AddBus(const string& bus){
		buses_.insert(string_view(bus));
	}
	const unordered_map<string, size_t>& GetDists(){
		return distancesToAnotherStops_;
	}
	const unordered_map<string, size_t>& GetDists() const {
		return distancesToAnotherStops_;
	}
	void AddStopDist(pair<string, size_t> sd){
		if (distancesToAnotherStops_.count(sd.first) < 1) {
			distancesToAnotherStops_.insert(move(sd));
		}
	}
	pair<const set<string_view>::iterator, const set<string_view>::iterator> GetAnswer(){
		return {buses_.begin(), buses_.end()};
	}

private:
	string name_;
	double latitude_;
	double longitude_;
	set<string_view> buses_ = set<string_view>();
	unordered_map<string, size_t> distancesToAnotherStops_;
};

template <class T>
class DataBase {
public:
	bool Add(T item) {
		string name = string(item.GetName());
		return db.insert(make_pair(move(name), move(item))).second;
	}
	unordered_map<string, T>& GetAccess() {
		return db;
	}
	const unordered_map<string, T>& GetAccess() const {
		return db;
	}

private:
	unordered_map<string, T> db;
};

enum class BusType {
	straight,
	circular
};
struct BusAnswer {
	explicit BusAnswer(int sc, int usc, double rl, double c) : stop_count(sc), unique_stop_count(usc), route_length(rl), curvature(c){}
	int stop_count;
	int unique_stop_count;
	double route_length;
	double curvature;
};

class Bus {
public:
	Bus() = delete;
	Bus(string name, BusType type, list <string> route) : name_(name), type_(type), route_(route){}
	BusAnswer GetAnswer(const DataBase<Stop>& stops) const {
		if (route_.size() < 2){throw runtime_error("route < 2");}
		double pathLength = 0.0;
		double geoLength = 0.0;
		set<string_view> set(route_.begin(), route_.end());
		size_t uniqueStopsCount = set.size();
		size_t stopsCount = route_.size();

		for (auto it = route_.begin(); it != route_.end(); it++) {
			if (it == route_.begin()) {continue;}
			auto itPrev = it;
			itPrev--;
			pathLength += stops.GetAccess().at(*itPrev).CalcPathLength(stops.GetAccess().at(*it));
			geoLength += stops.GetAccess().at(*itPrev).CalcGeoLength(stops.GetAccess().at(*it));
		}
		if (type_ == BusType::straight){
			geoLength *= 2;
			stopsCount = stopsCount * 2 - 1;
			for (auto it = route_.rbegin(); it != route_.rend(); it++) {
				if (it == route_.rbegin()) {continue;}
				auto itPrev = it;
				itPrev--;
				pathLength += stops.GetAccess().at(*itPrev).CalcPathLength(stops.GetAccess().at(*it));
			}
		}
		double curv = pathLength / geoLength;
		return BusAnswer(stopsCount, uniqueStopsCount, pathLength, curv);
	}
	const string& GetName() const {
		return name_;
	}
	BusType GetType() const {
		return type_;
	}
	const list<string>& GetRoute() const {
		return route_;
	}


private:
	string name_;
	BusType type_;
	list <string> route_;
};

enum class RequestType {
	AddStop,
	AddBus,
	FindBus,
	FindStop
};

struct Request {
	RequestType type;
	optional<Stop> stop;
	optional<Bus> bus;
	string name;
};

class TransportGuide {
public:
	Json::Document ProcessingJson(const Json::Document& json){
		for (auto& request : json.GetRoot().AsMap().at("base_requests").AsArray()){
			if (request.AsMap().at("type").AsString() == "Stop"){
				unordered_map<string, size_t> stopDists;
				if (request.AsMap().count("road_distances") > 0){
					for (auto& stop : request.AsMap().at("road_distances").AsMap()){
						stopDists.insert(make_pair(stop.first, static_cast<size_t>(stop.second.AsInt())));
					}
				}
				AddStop(Stop(request.AsMap().at("name").AsString(),
						request.AsMap().at("latitude").AsDouble(),
						request.AsMap().at("longitude").AsDouble(),
						move(stopDists)));
			} else if (request.AsMap().at("type").AsString() == "Bus"){
				list<string> route;
				for (auto& stop : request.AsMap().at("stops").AsArray()){
					route.push_back(stop.AsString());
				}
				BusType type = BusType::straight;
				if (request.AsMap().at("is_roundtrip").AsBool()){type = BusType::circular;}
				AddBus(Bus(request.AsMap().at("name").AsString(),
						type,
						move(route)));
			}
		}
		bus_wait_time_ = json.GetRoot().AsMap().at("routing_settings").AsMap().at("bus_wait_time").AsInt();
		bus_velocity_  = json.GetRoot().AsMap().at("routing_settings").AsMap().at("bus_velocity").AsDouble();
		FillingStops();
		Graph::Router<Graph::EdgeWeight> router = BuildRouter();
		vector<Json::Node> result;
		for (auto& request : json.GetRoot().AsMap().at("stat_requests").AsArray()){
			if (request.AsMap().at("type").AsString() == "Stop"){
				optional<pair<const set<string_view>::iterator, const set<string_view>::iterator>> answer = FindStop(request.AsMap().at("name").AsString());
				map<string, Json::Node> res;
				res.emplace("request_id", Json::Node(request.AsMap().at("id").AsInt()));
				if(answer){
					vector<Json::Node> v;
					for(auto it = answer->first; it != answer->second; it++){
						v.push_back(Json::Node(string(*it)));
					}
					res.emplace("buses", Json::Node(move(v)));
				} else {
					res.emplace("error_message", Json::Node(string("not found")));
				}
				result.push_back(Json::Node(move(res)));
			} else if (request.AsMap().at("type").AsString() == "Bus"){
				map<string, Json::Node> res;
				res.emplace("request_id", Json::Node(request.AsMap().at("id").AsInt()));
				optional<BusAnswer> answer = FindBus(request.AsMap().at("name").AsString());
				if (answer){
					res.emplace("route_length", Json::Node(answer->route_length));
					res.emplace("curvature", Json::Node(answer->curvature));
					res.emplace("stop_count", Json::Node(answer->stop_count));
					res.emplace("unique_stop_count", Json::Node(answer->unique_stop_count));
				} else {
					res.emplace("error_message", Json::Node(string("not found")));
				}
				result.push_back(Json::Node(move(res)));
			} else if (request.AsMap().at("type").AsString() == "Route") {
				map<string, Json::Node> res;
				res.emplace("request_id", Json::Node(request.AsMap().at("id").AsInt()));
				optional<Graph::Router<Graph::EdgeWeight>::RouteInfo> routeInfo = router.BuildRoute(stop_to_id_.at(request.AsMap().at("from").AsString()),
																						 stop_to_id_.at(request.AsMap().at("to").AsString()));
				if (!routeInfo){
					res.emplace("error_message", Json::Node(string("not found")));
				} else {
					if(!res.emplace("total_time", Json::Node(routeInfo->weight.weight)).second){throw runtime_error("cant emplace total time");};
					vector<Json::Node> items;
					for (size_t i = 0; i < routeInfo->edge_count; i++){
						if (routeInfo->weight.weight == 0.0){break;}
						auto edge = graph_.GetEdge(router.GetRouteEdge(routeInfo->id, i));

						map<string, Json::Node> waitItem;
						waitItem.emplace("type", Json::Node(string("Wait")));
						waitItem.emplace("time", Json::Node(bus_wait_time_));
						waitItem.emplace("stop_name", string(id_to_stop_.at(edge.from)));
						items.push_back(move(waitItem));

						map<string, Json::Node> busItem;
						busItem.emplace("type", Json::Node(string("Bus")));
						busItem.emplace("time", Json::Node(edge.weight.weight - bus_wait_time_));
						busItem.emplace("bus", Json::Node(edge.weight.bus));
						busItem.emplace("span_count", Json::Node(int(edge.weight.stops_count)));
						items.push_back(move(busItem));
					}
					res.emplace("items", Json::Node(move(items)));
				}
				if(res.count("error_message") < 1 && res.count("total_time") < 1) {throw runtime_error("bad answer");}
				result.push_back(Json::Node(move(res)));
			}
		}
		return Json::Document(Json::Node(move(result)));
	}
	bool AddStop(Stop stop) {
		return stops_.Add(move(stop));
	}
	bool AddBus(Bus bus) {
		return buses_.Add(move(bus));
	}

	optional<pair<const set<string_view>::iterator, const set<string_view>::iterator>> FindStop(const string& name){
		if (stops_.GetAccess().count(name) < 1){return nullopt;}
		return stops_.GetAccess().at(name).GetAnswer();
	}

	optional<BusAnswer> FindBus(const string& name){
		if (buses_.GetAccess().count(name) < 1){return nullopt;}
		return buses_.GetAccess().at(name).GetAnswer(stops_);
	}


	void FillingStops(){
		for (auto& bus : buses_.GetAccess()){
			for (auto& stop : bus.second.GetRoute()){
				stops_.GetAccess().at(stop).AddBus(bus.second.GetName());
			}
		}
		for (auto& stop : stops_.GetAccess()){
			for (const auto& stopDist: stop.second.GetDists()){
				if(stops_.GetAccess().count(stopDist.first) < 1 ) {throw runtime_error("fail trying add stopDist to stop");}
				stops_.GetAccess().at(stopDist.first).AddStopDist(make_pair(stop.first, stopDist.second));
			}
		}
	}
	Graph::Router<Graph::EdgeWeight> BuildRouter(){
		BuildVertexIdMaps();
		graph_ = Graph::DirectedWeightedGraph<Graph::EdgeWeight>(stops_.GetAccess().size());
		for(auto& stop : stops_.GetAccess()){
			graph_.AddEdge({stop_to_id_.at(stop.first), stop_to_id_.at(stop.first), Graph::EdgeWeight(0)});
		}
		for(auto& bus : buses_.GetAccess()){
			for (auto it_from = bus.second.GetRoute().begin(); it_from != bus.second.GetRoute().end(); it_from++){
				for (auto it_to = it_from; ++it_to != bus.second.GetRoute().end();){
					double mins = 0.0;
					for(auto it = it_from; it != it_to; it++){
						auto next_it = it;
						next_it++;
						mins += stops_.GetAccess().at(*it).CalcPathLength(stops_.GetAccess().at(*next_it)) / (bus_velocity_ * 1000.0 / 60.0);
					}
					if (mins == 0.0){
						graph_.AddEdge({stop_to_id_.at(*it_from), stop_to_id_.at(*it_to), Graph::EdgeWeight(0)});
					} else {
						graph_.AddEdge({stop_to_id_.at(*it_from), stop_to_id_.at(*it_to),
							           Graph::EdgeWeight(mins + static_cast<double>(bus_wait_time_), bus.first, distance(it_from, it_to))});
					}
				}
			}
			if(bus.second.GetType() == BusType::straight){
				for (auto it_from = bus.second.GetRoute().rbegin(); it_from != bus.second.GetRoute().rend(); it_from++){
					for (auto it_to = it_from; ++it_to != bus.second.GetRoute().rend();){
						double mins = 0.0;
						for(auto it = it_from; it != it_to; it++){
							auto next_it = it;
							next_it++;
							mins += stops_.GetAccess().at(*it).CalcPathLength(stops_.GetAccess().at(*next_it)) / (bus_velocity_ * 1000.0 / 60.0);
						}
						if (mins == 0.0){
							graph_.AddEdge({stop_to_id_.at(*it_from), stop_to_id_.at(*it_to), Graph::EdgeWeight(0)});
						} else {
							graph_.AddEdge({stop_to_id_.at(*it_from), stop_to_id_.at(*it_to),
								           Graph::EdgeWeight(mins + static_cast<double>(bus_wait_time_), bus.first, distance(it_from, it_to))});
						}
					}
				}
			}
		}
		return Graph::Router<Graph::EdgeWeight>(graph_);
	}
	void BuildVertexIdMaps(){
		size_t index = 0;
		for (auto& stop : stops_.GetAccess()){
			id_to_stop_.emplace(index++, string_view(stop.first));
		}
		for (auto& id : id_to_stop_){
			stop_to_id_.emplace(id.second, id.first);
		}
	}
private:
	DataBase<Stop> stops_;
	DataBase<Bus> buses_;
	int bus_wait_time_;
	double bus_velocity_;
	Graph::DirectedWeightedGraph<Graph::EdgeWeight> graph_ = Graph::DirectedWeightedGraph<Graph::EdgeWeight>(0);
	unordered_map<size_t, string_view> id_to_stop_;
	unordered_map<string_view, size_t> stop_to_id_;
};
