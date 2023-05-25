//Block缓存
#ifndef BLOCKCACHE
#define BLOCKCACHE

#include"../include/parameter.h"
#include<map>
#include<vector>

const long MAX_CACHE_SIZE = 16;

class BlockCache
{
public:
    char cache_[BLOCK_SIZE];
    int block_id_;
    bool modified_;
    int lock_;

public:
    BlockCache();
    BlockCache(int block_id);
    ~BlockCache();

    char *data();
};

#endif

#ifndef BLOCKCACHEMGR
#define BLOCKCACHEMGR

class BlockCacheMgr 
{
public:
    std::vector<int> cache_id_queue_;//有限cache块（这里缓存算法用FIFO)
    std::map<int, BlockCache> cache_map_;//缓存块号-实际块号

public:
    BlockCache *get_block_cache(int block_id);//获取block cache
};

#endif