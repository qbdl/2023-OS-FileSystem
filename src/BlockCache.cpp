#include"../include/BlockCache.h"
#include "../include/fs.h"

extern FileSystem fs;

BlockCache::BlockCache() {}

BlockCache::BlockCache(int block_id) 
{
    modified_ = false;
    block_id_ = block_id;
    lock_ = 0;
    fs.read_block(block_id_, cache_);
}

//释放blockcache时写回
BlockCache::~BlockCache() 
{
    if (modified_)
        fs.write_block(block_id_, cache_);
}

char *BlockCache::data()
{
    return cache_;
}

BlockCache *BlockCacheMgr::get_block_cache(int block_id) 
{
    if (cache_map_.find(block_id) == cache_map_.end()) {//原来没有对应缓存
        if (cache_id_queue_.size() == MAX_CACHE_SIZE) {//缓存已满-替换
            int old_block_id = cache_id_queue_.front();
            cache_id_queue_.erase(cache_id_queue_.begin());
            cache_map_.erase(old_block_id);
        }
        cache_id_queue_.push_back(block_id);
        cache_map_[block_id] = BlockCache(block_id);    
        //TODO:修改，访问时间等暂未处理    
    }
    return &cache_map_[block_id];
}
