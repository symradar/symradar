#include "MetadataExtractor.h"

std::map<Value*, string> collect_local_variable_metadata(Function& F){
    std::map<Value*, string> valueNameMap;
    for (auto &BB: F){
        for (auto &I: BB){

            if(DbgValueInst *dbgValue = dyn_cast<DbgValueInst>(&I)){
                if(MetadataAsValue* val = dyn_cast_or_null<MetadataAsValue>(dbgValue->getOperand(0))){
                    if(ValueAsMetadata* valueAsMetadata = dyn_cast<ValueAsMetadata>(val->getMetadata())){

                        Value* key = valueAsMetadata->getValue();
                        if(key != nullptr){
                            string name = dbgValue->getVariable()->getRawName()->getString();
                            valueNameMap[key] = name;
                        }
                    }
                }
            } else if (DbgDeclareInst* dbgDecl = dyn_cast<DbgDeclareInst>(&I)){
                if(MetadataAsValue* val = dyn_cast_or_null<MetadataAsValue>(dbgDecl->getOperand(0))){
                    if(ValueAsMetadata* valueAsMetadata = dyn_cast<ValueAsMetadata>(val->getMetadata())){
                        Value* key = valueAsMetadata->getValue();
                        if(key != nullptr){
                            string name = dbgDecl->getVariable()->getRawName()->getString();
                            valueNameMap[key] = name;
                        }
                    }
                }
            }
        }// end for(I)
    }// end for(BB)
    return valueNameMap;
}

std::map<Value*, struct variable> collect_local_variable_full_metadata(Function& F){
    std::map<Value*, struct variable> valueNameMap;
    for (auto &BB: F){
        for (auto &I: BB){

            if(DbgValueInst *dbgValue = dyn_cast<DbgValueInst>(&I)){
                if(MetadataAsValue* val = dyn_cast_or_null<MetadataAsValue>(dbgValue->getOperand(0))){
                    if(ValueAsMetadata* valueAsMetadata = dyn_cast<ValueAsMetadata>(val->getMetadata())){

                        Value* key = valueAsMetadata->getValue();
                        if(key != nullptr){
                            string name = dbgValue->getVariable()->getRawName()->getString();
                            int line = dbgValue->getVariable()->getLine();
                            DITypeRef type = dbgValue->getVariable()->getType();
                            struct variable var = {name, line, type};
                            valueNameMap[key] = var;
                        }
                    }
                }
            } else if (DbgDeclareInst* dbgDecl = dyn_cast<DbgDeclareInst>(&I)){
                if(MetadataAsValue* val = dyn_cast_or_null<MetadataAsValue>(dbgDecl->getOperand(0))){
                    if(ValueAsMetadata* valueAsMetadata = dyn_cast<ValueAsMetadata>(val->getMetadata())){
                        Value* key = valueAsMetadata->getValue();
                        if(key != nullptr){
                            string name = dbgDecl->getVariable()->getRawName()->getString();
                            int line = dbgDecl->getVariable()->getLine();
                            DITypeRef type = dbgDecl->getVariable()->getType();
                            struct variable var = {name, line, type};
                            valueNameMap[key] = var;
                        }
                    }
                }
            }
        }// end for(I)
    }// end for(BB)
    return valueNameMap;
}

string get_va_nm(Function *F, Value *param, map<Value *, string> &valueNameMap){
    int instCount = 0;
    for (BasicBlock& BB : *F) {
        instCount += std::distance(BB.begin(), BB.end());
    }
    int i = 0;
    while(i < instCount){
        i++;
        if(valueNameMap.count(param)){
            return valueNameMap[param];
        } else {

            if(BitCastInst* bc = dyn_cast<BitCastInst>(param)){
                param = dyn_cast<Instruction>(bc->getOperand(0));
                continue;
            }
            if(AllocaInst* al = dyn_cast<AllocaInst>(param)){
                param = al->getArraySize();
                continue;
            }
            if(SExtInst* se = dyn_cast<SExtInst>(param)){
                if(Value* oper = dyn_cast<Instruction>(se->getOperand(0))){
                    param = oper;
                    continue;
                } else {
                    break;
                }

            }

        }
    }
    static int counter = 0;
    string tmp_name = "tmp_" + F->getName().str() + "_" + std::to_string(counter);
    counter++;
    return tmp_name;
}


