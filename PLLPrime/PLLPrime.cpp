// PLLPrime.cpp : �f������񏈗��ŋ��߂�e�X�g�v���O����
//PPL�̕���A���S���Y�� CodeZine ���
//http://codezine.jp/article/detail/7632
#include "stdafx.h"
#include <array>
#include <vector>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>

#include <ppl.h>	//���񏈗��p
#include <concurrent_vector.h>
using namespace concurrency;

//int _tmain(int argc, _TCHAR* argv[])
//{
//	return 0;
//}

//
//�V�[�P���V�����ȏ���
//

bool is_prime(unsigned int n)
{
	if( n < 2U ){// 0 1 �͑f���ł͂Ȃ�
		return false;
	}
	//2 �ȏ� n �����̐� i ��
	for( unsigned int i = 2U; i < n; i++ ){
		// n ������؂�Ȃ�An �͑f������Ȃ�
		if( n % i== 0 ){
			return false;
		}
	}
	return true;
}

//2,3,5 �Ŋ���؂�Ȃ����� 30�̔{��+1,7,11
unsigned int nth_prime_candidate(unsigned int x,unsigned int y)
{
	static std::array<unsigned int,8> bias = {1,7,11,13,17,19,23,29};
	return	30*x + bias[y];
}

inline unsigned int nth_prime_candidate( unsigned int n ){
	return	nth_prime_candidate( n/8, n%8 );
}

//�f���T�� ������ prime �� pirmes �Ɋi�[����
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
//	���s����
//

//
//parallel_for(first, last, step, func)	step���ȗ������1�Ƃ݂Ȃ���܂�
//parallel_for �֐�
//http://msdn.microsoft.com/ja-jp/library/dd505035.aspx
//���@: parallel_for ���[�v���L�q����
//http://msdn.microsoft.com/ja-jp/library/dd728073.aspx

void	prime_parallel_for(unsigned int limit,std::vector<unsigned int>&primes)
{
	critical_section	cs;
	parallel_for(0U, limit, [&primes, &cs](unsigned int i) {
		unsigned int n = nth_prime_candidate(i);
		if (is_prime(n)){
			critical_section::scoped_lock	gurad(cs);
			std::cout << n << " is primes" << std::endl;
			primes.push_back(n);
		}
	});
}

//
//parallel_for_each�֐�
//http://msdn.microsoft.com/ja-jp/library/dd492857.aspx
//���@: parallel_for_each ���[�v���L�q����
//http://msdn.microsoft.com/ja-jp/library/dd728080.aspx
//
void	prime_parallel_for_each(unsigned int limit, std::vector<unsigned int>&primes)
{
	std::vector<unsigned int>	candidates(limit);

	//http://msdn.microsoft.com/ja-jp/library/jj651033.aspx
	iota(begin(candidates), end(candidates),0U);

	parallel_transform(begin(candidates), end(candidates), begin(candidates), [](unsigned int n){ return nth_prime_candidate(n); });

	critical_section cs;
	parallel_for_each( begin(candidates),end(candidates),
		[&primes, &cs](unsigned int n){
			if (is_prime(n)){
				critical_section::scoped_lock	gurad(cs);
				std::cout << n << " is primes" << std::endl;
				primes.push_back(n);
			}
		});
}

void	prime_parallel_invoke(unsigned int limit,std::vector<unsigned int>& primes)
{
	critical_section cs;//�r��������s��
	limit /= 8;
	auto prime_task  = [&primes,&cs,limit](unsigned int bias){
		for( unsigned int i = 0; i < limit; ++i ){
			unsigned int n = nth_prime_candidate( i, bias );
			if( is_prime( n ) ){
				critical_section::scoped_lock	guard(cs);//gurad ���L���Ȃ��o�����p
				std::cout << n << " is primes" << std::endl;
				primes.push_back( n );
			}
		}
	};
	//8 �� lambda �������ɕ]��
	//parallel_invoke �֐�
	//http://msdn.microsoft.com/ja-jp/library/dd504887.aspx
	//�uparallel_invoke(a, b, c �c�c)�����a(), b(), c()�c�c������Ɏ��s����A�S���I���̂�҂��Ă���܂��i�����ɗ^����֐��I�u�W�F�N�g�͍ő�10�j�v
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

///
///parallel_invoke()��concurrent_vector<>���g������
///
void	prime_parallel_invoke_concurrent_vector(unsigned int limit, std::vector<unsigned int>& primes)
{
	concurrent_vector<unsigned int>	cprimes;//concurrent_vector<>�̐擪�v�f�̃A�h���X��z�����Ɏg���Ă͂����Ȃ�
	limit /= 8;
	auto prime_task = [&cprimes, limit](unsigned int bias){
		for (unsigned int i = 0; i < limit; ++i){
			unsigned int n = nth_prime_candidate(i, bias);
			if (is_prime(n)){
				std::cout << n << " is primes" << std::endl;
				cprimes.push_back(n);
			}
		}
	};
	//8 �� lambda �������ɕ]��
	//parallel_invoke �֐�
	//http://msdn.microsoft.com/ja-jp/library/dd504887.aspx
	//�uparallel_invoke(a, b, c �c�c)�����a(), b(), c()�c�c������Ɏ��s����A�S���I���̂�҂��Ă���܂��i�����ɗ^����֐��I�u�W�F�N�g�͍ő�10�j�v
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
	primes.assign(begin(cprimes), end(cprimes));
}

int main()
{
	const unsigned int LIMIT = 40000;
	std::vector<unsigned int> primes;
	std::vector<unsigned int>::size_type count;
	std::chrono::milliseconds duration;

	primes.clear();
//	duration = mesure_exec(&prime_sequential,LIMIT,primes);
//	duration = mesure_exec(&prime_parallel_invoke,LIMIT,primes);
//	duration = mesure_exec(&prime_parallel_invoke_concurrent_vector, LIMIT, primes);
//	duration = mesure_exec(&prime_parallel_for, LIMIT, primes);
	duration = mesure_exec(&prime_parallel_for_each, LIMIT, primes);
	count = primes.size();
	std::cout << count << " primes found ." << std::endl;
	std::cout << "------------- sequential " << std::endl;
	std::cout << duration .count()<< "[ms]" << std::endl;
}