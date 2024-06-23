CC = g++
CXX_FLAGS = -std=c++17


test_lambda : test_lambda.cc
	$(CC) $^ -o $@ $(CXX_FLAGS)

test_structured_binding : 结构化绑定.cc
	$(CC) $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f test_lambda

.PHONY:	\
	test_lambda					\
	test_structured_binding		\
	clean						\