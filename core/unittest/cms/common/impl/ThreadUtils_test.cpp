//
// Created by 韩呈杰 on 2024/5/11.
//
//
// Created by 韩呈杰 on 2023/5/6.
//
#include <gtest/gtest.h>
#include "common/ThreadUtils.h"
#include "common/SyncQueue.h"
#include "common/BoostStacktraceMt.h"
#include "common/Common.h" // GetThisThreadId
#ifdef WIN32
#include "common/StringUtils.h"
#endif

#include <thread>
#include <boost/stacktrace.hpp>

TEST(SicOtherThreadCallStackTest, CallStack) {
    std::cout << boost::stacktrace::stacktrace();
}

#if !defined(WITHOUT_MINI_DUMP)
TEST(SicThreadStackTest, DefaultConstruct) {
    tagThreadStack o;
    EXPECT_EQ(o.stackTrace, "");
}
#endif

// 未搞定apple下线程遍历问题，跳过该用例
#if !defined(WITHOUT_MINI_DUMP) && !defined(__APPLE__) && !defined(__FreeBSD__)
void threadFunc___1(SyncQueue<bool> queueEnter, SyncQueue<bool> queueExit) {
	SetThreadName(__FUNCTION__);
	queueEnter << true;
	queueExit.Wait();
}

void threadFunc___2(SyncQueue<bool> queueEnter, SyncQueue<bool> queueExit) {
	SetThreadName(__FUNCTION__);
	queueEnter << true;
	queueExit.Wait();
}

TEST(SicOtherThreadCallStackTest, Print) {
	SyncQueue<bool> queueExit(2);
	EXPECT_EQ((size_t)2, queueExit.Capacity());

	SyncQueue<bool> queueEntered;
	std::vector<std::shared_ptr<std::thread>> threads;
	threads.push_back(std::make_shared<std::thread>(threadFunc___1, queueEntered, queueExit));
	threads.push_back(std::make_shared<std::thread>(threadFunc___2, queueEntered, queueExit));

	for (size_t i = 0; i < threads.size(); i++) {
        queueEntered.Wait();
    }

	std::list<std::string> lstTrace;
    WalkThreadStack([&lstTrace](int index, const tagThreadStack &s) {
        lstTrace.push_back(s.str(index));
    });

	for (size_t i = 0; i < threads.size(); i++) {
		queueExit << true; // 线程退出
	}

	for (const auto& s : lstTrace) {
		std::cout << s << std::endl;
		// EXPECT_NE(stack.stackTrace.find("threadFunc"), std::string::npos);
	}
	EXPECT_GE(lstTrace.size(), threads.size());

	for (auto& thread : threads) {
		if (thread->joinable()) {
			thread->join();
		}
	}
}
#endif

#if !defined(WITHOUT_MINI_DUMP) && !defined(WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)

TEST(SicOtherThreadCallStackTest, pthread_hook) {
	SyncQueue<bool> waitQueue, startedQueue{ 1 };
	pthread_t myThread = 0;
	int myThreadId = 0;
	std::thread thread([&]() {
		myThread = pthread_self();
		myThreadId = GetThisThreadId();
		startedQueue << true;
		waitQueue.Wait();
		});
	startedQueue.Wait(); // 确保线程已运行

	{
		std::map<int, pthread_t> threads;
		EnumThreads(threads);
		std::cout << "thread count: " << threads.size() << std::endl;
		EXPECT_NE(0, pthread_equal(threads.find(myThreadId)->second, myThread));
	}
	waitQueue << true;
	thread.join();

	{
		std::map<int, pthread_t> threads;
		EnumThreads(threads);
		EXPECT_EQ(threads.find(myThreadId), threads.end());
	}
}

#endif

#ifdef WIN32
extern bool IsSupportThreadName();
#endif

TEST(SicOtherThreadCallStackTest, ThreadName) {
    SyncQueue<bool> waitQueue, startedQueue{ 1 };
    int myThreadId = 0;
    std::thread thread([&]() {
        std::cout << "ThreadName: " << GetThreadName(GetThisThreadId()) << std::endl;
        SetThreadName("thread-test");
        myThreadId = GetThisThreadId();
        startedQueue << true;
        waitQueue.Wait();
    });
    startedQueue.Wait(); // 确保线程已运行

    {
        std::string name = GetThreadName(myThreadId);
        std::cout << __FILE__ << ":" << __LINE__ << ": threadName: " << name << std::endl;
#ifndef WIN32
        EXPECT_EQ(name, "thread-test");
#else
        if (IsSupportThreadName()) {
			EXPECT_EQ(name, "thread-test");
		}
		else {
			EXPECT_EQ(name, "");
		}
#endif
    }
    waitQueue << true;
    thread.join();
}

#ifdef WIN32
TEST(SicOtherThreadCallStackTest, ThreadNameCn) {
	if (IsSupportThreadName()) {
		SyncQueue<bool> waitQueue, startedQueue{ 1 };
		int targetThreadId = 0;
		std::thread thread([&]() {
			SetThreadName("线程1");
			targetThreadId = GetThisThreadId();
			startedQueue << true;
			waitQueue.Wait();
			});
		startedQueue.Wait(); // 确保线程已运行

		std::string name = GetThreadName(targetThreadId);
		std::cout << __FILE__ << ":" << __LINE__ << ": threadName: " << UTF8ToGBK(name) << std::endl;
		EXPECT_EQ(name, "线程1");

		waitQueue << true;
		thread.join();
	}
}
#endif

TEST(SicOtherThreadCallStackTest, ThreadId) {
    // Windows: ==
    // Linux  : !=
    // macOS  : !=
    std::cout << "GetThisThreadId: " << GetThisThreadId() << std::endl
              << "this_thread::Id: " << std::this_thread::get_id() << std::endl;
}
