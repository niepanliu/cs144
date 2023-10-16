#include <stdexcept>
#include <iostream>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if(data.empty() || capacity_-bufferstream.size() == 0)
    return ;
  uint64_t data_lenth = data.length();
  if(capacity_-bufferstream.size() < data_lenth) 
    data_lenth = capacity_-bufferstream.length();
  data = data.substr(0, data_lenth);
  bufferstream += std::move(data);
  byte_write_number_ += data_lenth;
  // if(available_capacity() == 0 || data.empty())
  //   return ;
  // auto const n = min(available_capacity(), data.size());
  // if(n <data.size())
  //   data = data.substr(0,n);
  // data_queue_.push_back(std::move(data));
  // view_queue_.emplace_back(data_queue_.back().c_str(),n);
  // num_bytes_buffered_ += n;
  // num_bytes_pushed_ += n;
}

void Writer::close()
{
  end_ = true;
  // is_closed_ = true;
}

void Writer::set_error()
{
  error_ = true;
  // has_error_ = true;
}

bool Writer::is_closed() const
{
  return end_ ;
  // return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - bufferstream.length();
  // return capacity_ - num_bytes_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return byte_write_number_;
  // return num_bytes_pushed_;
}

string_view Reader::peek() const
{
  if(bufferstream.empty())
    return {};
  string_view view_queue_(bufferstream);
  return view_queue_;
  // if(view_queue_.empty())
  //   return {};
  // return view_queue_.front(); 
}

bool Reader::is_finished() const
{
  return (end_ && bufferstream.empty());
  // return is_closed_ && num_bytes_buffered_ == 0;
}

bool Reader::has_error() const
{
  return error_;
  // return has_error_;
}

void Reader::pop( uint64_t len )
{
  uint64_t mindata;
  if(bufferstream.length() > len)
    mindata = len;
  else
    mindata = bufferstream.length();
  byte_read_number_ += mindata;
  bufferstream.erase(0, mindata);

  // auto n = min(len , num_bytes_buffered_);
  // while (n>0)
  // {
  //   auto sz = view_queue_.front().size();
  //   if(n<sz)
  //   {
  //     view_queue_.front().remove_prefix(n);
  //     num_bytes_buffered_ -= n;
  //     num_bytes_popped_ += n;
  //     return;
  //   }
  //   view_queue_.pop_front();
  //   data_queue_.pop_front();
  //   n-=sz;
  //   num_bytes_buffered_ -= sz;
  //   num_bytes_popped_ +=sz;
  // }
  
}

uint64_t Reader::bytes_buffered() const
{
  return bufferstream.size();
  // return num_bytes_buffered_;
}

uint64_t Reader::bytes_popped() const
{
  return byte_read_number_;
  // return num_bytes_popped_;
}
