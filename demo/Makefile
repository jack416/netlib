VPATH = $(OBJ_OUTPUT_DIR) $(SOURCE_DIR) $(HEADER_DIR) 
OBJ_OUTPUT_DIR=./output
OUTPUT_DIR=../lib

$(shell mkdir $(OBJ_OUTPUT_DIR))
$(shell mkdir $(OUTPUT_DIR))

SOURCE_DIR=../Source
HEADER_DIR=../Include

CFLAGS= -O -g -fPIC -Wall -Wno-strict-aliasing -Wno-conversion-null -Wno-sign-compare -Wno-unused-value -Wno-delete-non-virtual-dtor -Wno-pointer-arith -D_REENTRANT -DUSE_APACHE -DNO_STRING_CIPHER 

WARNING_LEVEL += -O3 

INCLUDE = -I. -I$(HEADER_DIR) 

LIB = -lnsl -lc -lm -pthread -lstdc++ 

LIB += 

MAIN =

OUTPUT = 

OBJ_LIB = $(notdir $(patsubst %.cpp,%.o,$(wildcard $(SOURCE_DIR)/*.cpp))) 

DEPENDENCE = $(OBJ_LIB) 

OBJ = $(addprefix $(OBJ_OUTPUT_DIR)/, $(OBJ_LIB)) 


$(OUTPUT_DIR)/$(OUTPUT):$(MAIN)$(DEPENDENCE)
	ar -r $@ $(OBJ)


$(OBJ_LIB):%.o:%.cpp %.h
	$(CC) -c -o $(OBJ_OUTPUT_DIR)/$*.o $(CFLAGS)$(WARNING_LEVEL)$(INCLUDE) $(SOURCE_DIR)/$*.cpp


clean:
	-rm -f $(OUTPUT_DIR)/$(OUTPUT) $(OBJ_OUTPUT_DIR)/*.o
	
.PHONY: clean
