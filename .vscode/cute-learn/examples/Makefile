CUTLASS := ../../..
NVCC    := nvcc
FLAGS   := -I $(CUTLASS)/include -I $(CUTLASS)/tools/util/include -std=c++17 -Wno-deprecated-gpu-targets
BUILD   := build

# 按主题归档的示例(编号即学习顺序):
#   01_layout 02_tuple 03_coalesce 04_composition 05_complement 06_divide 07_product
# 用法: make run F=01_layout   (编译并运行, 产物进 build/)
#       make run F=06          (前缀匹配也行, 见下)
#       make all               (编译全部)
#       make clean             (清掉 build/)
F ?= 01_layout

# 允许用编号前缀简写: F=06 -> 找 06_*.cpp
SRC := $(firstword $(wildcard $(F)*.cpp) $(F).cpp)
BIN := $(BUILD)/$(basename $(notdir $(SRC)))

run: $(BIN)
	./$(BIN)

$(BIN): $(SRC) | $(BUILD)
	$(NVCC) $(FLAGS) $< -o $@

all: | $(BUILD)
	@for f in $(wildcard [0-9]*.cpp); do \
		echo "== $$f =="; \
		$(NVCC) $(FLAGS) $$f -o $(BUILD)/$${f%.cpp} || exit 1; \
	done

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: run all clean
