#ifndef __SH_COMM_DEF_H__
#define __SH_COMM_DEF_H__

#pragma pack(1)

#define sh_null 0

#define sh_typeof typeof
#define sh_offsetof(type, member) (((unsigned long)&(((type *)1)->member)) - 1)
#define sh_container_of(ptr, type, member) ((type*)((ptr) == 0 ? 0 : (((char*)(ptr)) - sh_offsetof(type, member))))

#define sh_size_add(a, b) ((unsigned long)(a) + (unsigned long)(b))
#define sh_size_sub(a, b) ((unsigned long)(a) - (unsigned long)(b))

#define sh_set_flag(mask, flag) ((mask) |= (flag))
#define sh_del_flag(mask, flag) ((mask) &= (~(flag)))
#define sh_is_flag_set(mask, flag) ((mask) & (flag))

#define sh_align_ceil(v, align_num) ((v) = ((v) + (align_num) - 1) / (align_num) * (align_num))

extern unsigned long g_shm_base;

class sh_ptr_common_t
{
	public:
		unsigned long m_ptr;
		sh_ptr_common_t():m_ptr(sh_null){}
		sh_ptr_common_t(void *ptr)
		{
			if ((unsigned long)ptr < g_shm_base)
			{
				m_ptr = sh_null;
			}
			else
			{
				m_ptr = (unsigned long)(ptr) - g_shm_base;
			}
		}
		sh_ptr_common_t(const sh_ptr_common_t &ptr)
		{
			m_ptr = ptr.m_ptr;
		}
//		void* get_real_addr()
//		{
//			if (m_ptr == sh_null)
//			{
//				return NULL;
//			}
//			return (void*)(m_ptr + g_shm_base);
//		}
//		long get_rel_addr()
//		{
//			return m_ptr;
//		}
		int operator==(sh_ptr_common_t sh_ptr_common)
		{
			return (m_ptr == sh_ptr_common.m_ptr);
		}
		int operator==(void *ptr)
		{
			if (ptr == NULL)
			{
				return (m_ptr == sh_null);
			}
			return (m_ptr + g_shm_base == (unsigned long)(ptr));
		}
		int operator!=(sh_ptr_common_t sh_ptr_common)
		{
			return (m_ptr != sh_ptr_common.m_ptr);
		}
		int operator!=(void *ptr)
		{
			if (ptr == NULL)
			{
				return (m_ptr != sh_null);
			}
			return (m_ptr + g_shm_base != (unsigned long)ptr);
		}
		sh_ptr_common_t operator=(sh_ptr_common_t sh_ptr_common)
		{
			m_ptr = sh_ptr_common.m_ptr;
			return *this;
		}
		sh_ptr_common_t operator=(void *ptr)
		{
			if ((unsigned long)ptr < g_shm_base)
			{
				m_ptr = sh_null;
			}
			else
			{
				m_ptr = (unsigned long)(ptr) - g_shm_base;
			}
			return *this;
		}
		operator char*()
		{
			if (m_ptr == sh_null)
			{
				return (char*)NULL;
			}
			return (char*)(m_ptr + g_shm_base);
		}
};

template <typename T>
class sh_ptr_t : public sh_ptr_common_t
{
	public:
		sh_ptr_t()
		{
			m_ptr = sh_null;
		}
		sh_ptr_t(void *ptr)
		{
			if ((unsigned long)ptr < g_shm_base)
			{
				m_ptr = sh_null;
			}
			else
			{
				m_ptr = (unsigned long)(ptr) - g_shm_base;
			}
		}
		sh_ptr_t(const sh_ptr_common_t &ptr)
		{
			m_ptr = ptr.m_ptr;
		}
		T* operator->()
		{
			if (m_ptr == sh_null)
			{
				return (T*)(NULL);
			}
			return (T*)(m_ptr + g_shm_base);
		}
		int operator==(sh_ptr_common_t sh_ptr_common)
		{
			return (m_ptr == sh_ptr_common.m_ptr);
		}
		int operator==(void *ptr)
		{
			if (ptr == NULL)
			{
				return (m_ptr == sh_null);
			}
			return (m_ptr + g_shm_base == (unsigned long)(ptr));
		}
		int operator!=(sh_ptr_common_t sh_ptr_common)
		{
			return (m_ptr != sh_ptr_common.m_ptr);
		}
		int operator!=(void *ptr)
		{
			if (ptr == NULL)
			{
				return (m_ptr != sh_null);
			}
			return (m_ptr + g_shm_base != (unsigned long)ptr);
		}
		sh_ptr_t<T> operator=(sh_ptr_common_t sh_ptr_common)
		{
			m_ptr = sh_ptr_common.m_ptr;
			return *this;
		}
		sh_ptr_t<T> operator=(void *ptr)
		{
			if ((unsigned long)ptr < g_shm_base)
			{
				m_ptr = sh_null;
			}
			else
			{
				m_ptr = (unsigned long)(ptr) - g_shm_base;
			}
			return *this;
		}
		T& operator[](int idx) const
		{
			return *(T*)(((T*)(m_ptr + g_shm_base)) + idx);
		}
		T& operator*()
		{
			if (m_ptr == sh_null)
			{
				//can do nothing
			}
			return *(T*)(m_ptr + g_shm_base);
		}
		operator T*()
		{
			if (m_ptr == sh_null)
			{
				return (T*)NULL;
			}
			return (T*)(m_ptr + g_shm_base);
		}
};

#pragma pack()

#endif
