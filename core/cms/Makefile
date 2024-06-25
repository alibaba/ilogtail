.PHONY: prepare sync clean test setup

sync: prepare
	./sync.sh

prepare:
	mkdir -p ../unittest/cms/unittest_data

clean:
	find .                -mindepth 1 -maxdepth 1 -type d -exec rm -rf {} +
	find ../unittest/cms  -mindepth 1 -maxdepth 1 -type d -exec rm -rf {} +

test:
	../../build/unittest/cms/cms_unittest --gtest_filter='*'

setup:
	if [ -z "$(DIST_DIR)" ] && [ ! -f "$(DIST_DIR)" ]; then echo "need DIST_DIR"; exit 1; fi 
	if [ ! -f "$(DIST_DIR)/local_data/conf" ]; then mkdir -p "$(DIST_DIR)/local_data/conf"; fi
	cp ./runtime/agent.properties $(DIST_DIR)/local_data/conf/agent.properties
	cp ./runtime/moduleTask.json $(DIST_DIR)/local_data/conf/moduleTask.json