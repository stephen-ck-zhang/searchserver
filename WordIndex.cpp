#include "./WordIndex.hpp"
#include <algorithm>
#include <iostream>

namespace searchserver {

WordIndex::WordIndex() {
  word_index_ = unordered_map<string, unordered_map<string, size_t>>();
}

size_t WordIndex::num_words() {
  size_t num_words = 0;
  for (auto word : word_index_) {
    num_words++;
  }

  return num_words;
}

void WordIndex::record(const string& word, const string& doc_name) {
  if (word_index_.find(word) == word_index_.end()) {
    word_index_[word] = unordered_map<string, size_t>();
  }
  if (word_index_[word].find(doc_name) == word_index_[word].end()) {
    word_index_[word][doc_name] = 0;
  }

  word_index_[word][doc_name]++;
}

vector<Result> WordIndex::lookup_word(const string& word) {
  vector<Result> result;

  if (word_index_.find(word) != word_index_.end()) {
    for (auto doc : word_index_[word]) {
      result.push_back(Result(doc.first, doc.second));
    }
  }

  // Sort the results with the highest rank first
  std::sort(result.begin(), result.end(),
            [](const Result& a, const Result& b) { return a.rank > b.rank; });

  return result;
}

vector<Result> WordIndex::lookup_query(const vector<string>& query) {
  vector<Result> results;

  for (auto word : query) {
    vector<Result> word_results = lookup_word(word);

    if (results.size() == 0) {
      results = word_results;
    } else {
      vector<Result> new_results;
      for (auto result : results) {
        for (auto word_result : word_results) {
          if (result.doc_name == word_result.doc_name) {
            new_results.push_back(
                Result(result.doc_name, result.rank + word_result.rank));
          }
        }
      }
      results = new_results;
    }
  }

  // Sort the results with the highest rank first
  std::sort(results.begin(), results.end(),
            [](const Result& a, const Result& b) { return a.rank > b.rank; });

  return results;
}

}  // namespace searchserver
