CUTLASS := ../..
NVCC    := nvcc
FLAGS   := -I $(CUTLASS)/include -I $(CUTLASS)/tools/util/include -std=c++17 -Wno-deprecated-gpu-targets

# 用法: make run F=layout   (编译并运行 layout.cpp)
#       make run           (F 默认为 layout)
F ?= layout

run: $(F)
	./$(F)

$(F): $(F).cpp
	$(NVCC) $(FLAGS) $< -o $@

clean:
	rm -f layout $(F)

.PHONY: run clean
