#include <string>
#include <stdexcept>
#include "syrpc/dispatcher/dispatcher.hpp"

Dispatcher::Dispatcher():handlers_map(){};
Dispatcher::~Dispatcher(){
    handlers_map = {};
}

void Dispatcher::register_handler(uint32_t procedure_id,Handler handler){
    handlers_map[procedure_id] = handler;

}

std::string Dispatcher::dispatch(uint32_t procedure_id,const std::string &payload){
    auto entry = handlers_map.find(procedure_id);

    if (entry!=handlers_map.end()){
        Handler handler = entry->second;
        return handler(payload);
    }else{
        throw std::runtime_error("handler with id "+ std::to_string(procedure_id)+ "procedure_id not found" );
    }
    return "";
}
