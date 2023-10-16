#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  // (void)first_index;
  // (void)data;
  if(is_last_substring == true)
    is_close = first_index + data.length();
  // (void)output;
  if(data.length() == 0) {
    if(is_close == index)
      output.close();
    return ;
  }
  if(output.available_capacity() == 0){
    return ;
  }
  if(output.available_capacity() + index < first_index)
    return ;
  buffer.resize(output.available_capacity(), 0);
  buffer_use.resize(output.available_capacity(), 0);
  if(first_index < index) {
    if(first_index+data.length() < index)
      return ;
    if(first_index+data.length() <= index + output.available_capacity()) {
      data = data.substr(index-first_index, data.length());
      insert_(0, data, data.length());
      push(output, output.available_capacity());
    }
    if(first_index+data.length() > index + output.available_capacity()) {
      data = data.substr(index-first_index, output.available_capacity());
      insert_(0, data, data.length());
      push(output, output.available_capacity());
    }
  } else {
    if(first_index+data.length() <= index + output.available_capacity()) {
      data = data.substr(0, data.length());
      insert_(first_index-index, data, data.length());
      push(output, output.available_capacity());
    }
    if(first_index+data.length() > index + output.available_capacity()) {
      data = data.substr(0, index + output.available_capacity() - first_index);
      insert_(first_index-index, data, data.length());
      push(output, output.available_capacity());
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return pending;
}

void Reassembler::insert_(uint64_t buffindex, string data, uint64_t len)
{
  for(uint64_t i = 0; i < len; i++) {
    buffer.at(i+buffindex) = data[i];
    if(buffer_use.at(i+buffindex) == false)
      pending++;
    buffer_use.at(i+buffindex) = true;
  }
}
void Reassembler::push(Writer& output, uint64_t len) 
{
  string temp;
  uint64_t i = 0;
  while (buffer_use[i] && i < len && i < buffer_use.size()) {
    temp.push_back(buffer[i]);
    i++;
  }
  output.push(temp);
  buffer.erase(buffer.begin(),buffer.begin()+i);
  buffer_use.erase(buffer_use.begin(),buffer_use.begin()+i);
  index += i;
  pending -= i;
  if(index == is_close && temp.length() != 0)
    output.close();
}


