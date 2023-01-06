#pragma once

#include "../UnrealscriptDebugger/ScriptDebugLogger.h"

#define SCRIPTOBJECTFULLPATH(objPtr) (objPtr ? objPtr->GetFullPath() : "None")
#define PRINTALLELEMENTS(type, printStmnt) auto value = *reinterpret_cast<type*>(propAddr); \
											if (prop->ArrayDim > 1) \
											{ \
												logger.out() << "[" << prop->ArrayDim << "] : [" << printStmnt; \
												for (int i = prop->ArrayDim - 1; i > 0; --i) \
												{ \
													propAddr += prop->ElementSize; \
													value = *reinterpret_cast<type*>(propAddr); \
													logger.out() << ", " << printStmnt; \
												} \
												logger.out() << "]\n"; \
											} \
											else \
											{ \
												logger.out() << ": " << printStmnt << "\n"; \
											} \

void PrintPropertyValues(ScriptDebugLogger& logger, BYTE* propsOffset, UStruct* const node, OutParmInfo* outParmInfo = nullptr) {
	BYTE* propAddr = nullptr;
	logger.IncreaseIndent();
	for (auto curChild = node->Children; curChild; curChild = curChild->Next) {
		if (!IsA<UProperty>(curChild)) {
			continue;
		}

		auto prop = static_cast<UProperty*>(curChild);
		if (prop->PropertyFlags & 0x400) //ReturnValue flag
		{
			continue;
		}
		if (prop->PropertyFlags & 0x100) //OutParm flag
		{
			for (auto curOutParm = outParmInfo; curOutParm; curOutParm = curOutParm->Next) {
				if (curOutParm->Prop == prop) {
					propAddr = curOutParm->PropAddr;
					break;
				}
			}
		} else {
			propAddr = propsOffset + prop->Offset;
		}

		if (propAddr == nullptr) {
			continue;
		}

		auto propClass = prop->Class;

		auto propName = prop->GetName();
		logger.indent() << propName;
		if (propClass == UIntProperty::StaticClass()) {
			PRINTALLELEMENTS(int, value)
		} else if (propClass == UFloatProperty::StaticClass()) {
				PRINTALLELEMENTS(float, value)
			} else if (propClass == UByteProperty::StaticClass()) {
					PRINTALLELEMENTS(BYTE, static_cast<int>(value))
				} else if (propClass == UBoolProperty::StaticClass()) {
						//TODO: this is wrong! need to use bitmask!
						PRINTALLELEMENTS(unsigned*, (value ? "True" : "False"))
					} else if (propClass == UNameProperty::StaticClass()) {
							PRINTALLELEMENTS(FName, value.Instanced())
						} else if (propClass == UStrProperty::StaticClass()) {
								PRINTALLELEMENTS(FString, "\"" << (value.Data ? value.Data : L"") << "\"")
							} else if (propClass == UStringRefProperty::StaticClass()) {
									PRINTALLELEMENTS(int, "$" << value)
								} else if (propClass == UArrayProperty::StaticClass()) {
										auto array = *reinterpret_cast<TArray<BYTE>*>(propAddr);
										auto array_property = static_cast<UArrayProperty*>(prop);
										prop = array_property->Inner;
										propClass = prop->Class;
										logger.indent() << ": " << array.Count << " Elements ";
										logger.out() << "[";
										propAddr = array.Data;
										if (propClass == UStructProperty::StaticClass()) {
											auto uStruct = static_cast<UStructProperty*>(prop)->Struct;
											logger.out() << " : ( StructType: " << uStruct->GetName() << ") [\n";
											for (int i = 0; i < array.Num(); ++i) {
												if (i > 0) {
													logger.indent() << ",\n";
												}
												PrintPropertyValues(*scriptDebugLogger, propAddr, uStruct);
												propAddr += prop->ElementSize;
											}
											logger.indent() << "]\n";
										} else {
											for (int i = 0; i < array.Num(); ++i) {
												if (i > 0) {
													logger.out() << ", ";
												}
												if (propClass == UIntProperty::StaticClass()) {
													auto value = *reinterpret_cast<int*>(propAddr);
													logger.out() << value;
												} else if (propClass == UFloatProperty::StaticClass()) {
													auto value = *reinterpret_cast<float*>(propAddr);
													logger.out() << value;
												} else if (propClass == UByteProperty::StaticClass()) {
													auto value = *reinterpret_cast<BYTE*>(propAddr);
													logger.out() << static_cast<int>(value);
												} else if (propClass == UBoolProperty::StaticClass()) {
													auto value = *reinterpret_cast<unsigned*>(propAddr);
													logger.out() << (value ? "True" : "False");
												} else if (propClass == UNameProperty::StaticClass()) {
													auto value = *reinterpret_cast<FName*>(propAddr);
													logger.out() << value.Instanced();
												} else if (propClass == UStrProperty::StaticClass()) {
													auto value = *reinterpret_cast<FString*>(propAddr);
													logger.out() << "\"" << (value.Data ? value.Data : L"") << "\"";
												} else if (propClass == UStringRefProperty::StaticClass()) {
													auto value = *reinterpret_cast<int*>(propAddr);
													logger.out() << "$" << value;
												} else if (propClass == UDelegateProperty::StaticClass()) {
													auto value = *reinterpret_cast<FScriptDelegate*>(propAddr);
													logger.out() << "(Function Name:" << value.FunctionName.Instanced() << ", Object: " << SCRIPTOBJECTFULLPATH(value.Object) << ")";
												} else if (propClass == UInterfaceProperty::StaticClass()) {
													auto value = *reinterpret_cast<FScriptInterface*>(propAddr);
													logger.out() << SCRIPTOBJECTFULLPATH(value.Object);
												} else if (IsA<UObjectProperty>(prop)) {
													auto value = *reinterpret_cast<UObject**>(propAddr);
													logger.out() << SCRIPTOBJECTFULLPATH(value);
												}
												propAddr += prop->ElementSize;
											}
											logger.out() << "]\n";
										}
									} else if (propClass == UDelegateProperty::StaticClass()) {
										PRINTALLELEMENTS(FScriptDelegate, "(Function Name : " << value.FunctionName.Instanced() << ", Object : " << SCRIPTOBJECTFULLPATH(value.Object) << ")")
									} else if (propClass == UInterfaceProperty::StaticClass()) {
											PRINTALLELEMENTS(FScriptInterface, SCRIPTOBJECTFULLPATH(value.Object))
										} else if (propClass == UStructProperty::StaticClass()) {
												auto uStruct = static_cast<UStructProperty*>(prop)->Struct;
												auto value = reinterpret_cast<UStructProperty*>(propAddr);
												if (prop->ArrayDim > 1) {
													logger.out() << "[" << prop->ArrayDim << "] : ( StructType: " << uStruct->GetName() << ") [\n";
													PrintPropertyValues(*scriptDebugLogger, propAddr, uStruct);
													for (int i = prop->ArrayDim - 1; i > 0; --i) {
														logger.indent() << ",\n";
														propAddr += prop->ElementSize;
														PrintPropertyValues(*scriptDebugLogger, propAddr, uStruct);
													}
													logger.indent() << "]\n";
												} else {
													logger.out() << "( StructType: " << uStruct->GetName() << ")\n";
													PrintPropertyValues(*scriptDebugLogger, propAddr, uStruct);
												}
											} else if (IsA<UObjectProperty>(prop)) {
												PRINTALLELEMENTS(UObject*, SCRIPTOBJECTFULLPATH(value))
											}

	}
	logger.DecreaseIndent();
}

