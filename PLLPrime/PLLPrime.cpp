// PLLPrime.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
#include "stdafx.h"
#include <array>
#include <vector>
#include <chrono>
#include <functional>
#include <iostream>

#include <ppl.h>	//並列処理用

using namespace concurrency;

//int _tmain(int argc, _TCHAR* argv[])
//{
//	return 0;
//}

//
//シーケンシャルな処理
//

bool is_prime(unsigned int n)
{
	if( n < 2U ){// 0 1 は素数ではない
		return false;
	}
	//2 以上 n 未満の数 i が
	for( unsigned int i = 2U; i < n; i++ ){
		// n を割り切るなら、n は素数じゃない
		if( n % i== 0 ){
			return false;
		}
	}
	return true;
}

//2,3,5 で割り切れない数は 30の倍数+1,7,11
unsigned int nth_prime_candidate(unsigned int x,unsigned int y)
{
	static std::array<unsigned int,8> bias = {1,7,11,13,17,19,23,29};
	return	30*x + bias[y];
}

inline unsigned int nth_prime_candidate( unsigned int n ){
	return	nth_prime_candidate( n/8, n%8 );
}

//素数探し 見つけた prime は pirmes に格納する
void prime_sequential(unsigned int limit, std::vector<unsigned int>& primes)
{
	for(unsigned int i=0;i < limit; ++i ){
		unsigned int n = nth_prime_candidate(i);
		if( is_prime(n) ){
			std::cout << n << " is primes" << std::endl;
			primes.push_back(n);
		}
	}
}

inline std::chrono::milliseconds mesure_exec(const std::function<void(unsigned int,std::vector<unsigned int>&)>& fun,unsigned int limit,std::vector<unsigned int>& result)
{
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	fun(limit,result);
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
}


//
//	並行処理
//

void	prime_parallel_invoke(unsigned int limit,std::vector<unsigned int>& primes)
{
	critical_section cs;//排他制御を行う
	limit /= 8;
	auto prime_task  = [&primes,&cs,limit](unsigned int bias){
		for( unsigned int i = 0; i < limit; ++i ){
			unsigned int n = nth_prime_candidate( i, bias );
			if( is_prime( n ) ){
				critical_section::scoped_lock	guard(cs);//gurad が有効なら手出し無用
				std::cout << n << " is primes" << std::endl;
				primes.push_back( n );
			}
		}
	};
	//8 つの lambda 式を並列に評価
	parallel_invoke(
		[&](){ prime_task(0); },
		[&](){ prime_task(1); },
		[&](){ prime_task(2); },
		[&](){ prime_task(3); },
		[&](){ prime_task(4); },
		[&](){ prime_task(5); },
		[&](){ prime_task(6); },
		[&](){ prime_task(7); }
	);
}

int main()
{
	const unsigned int LIMIT = 40000;
	std::vector<unsigned int> primes;
	std::vector<unsigned int>::size_type count;
	std::chrono::milliseconds duration;

	primes.clear();
//	duration = mesure_exec(&prime_sequential,LIMIT,primes);
	duration = mesure_exec(&prime_parallel_invoke,LIMIT,primes);
	count = primes.size();
	std::cout << count << " primes found ." << std::endl;
	std::cout << "------------- sequential " << std::endl;
	std::cout << duration .count()<< "[ms]" << std::endl;
}