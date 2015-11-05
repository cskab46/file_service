#include "server_election.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <atomic>
#include <thread>
#include <future>
#include <chrono>

using namespace std::chrono;

#include "server/ordered_lock.h"

#include "connection.h"
#include "message.h"
#include "groups.h"

const double kElectionTimeout = 1000;

/**
 * @brief Performs a bully election, sending election messages to all rivals and
 * waiting for answers. If no answer comes, consider election as won.
 * @param connections identifiers whose priorities are greater than current.
 * @return true if the election is won. false otherwise.
 */
bool BullyElection(const vector<string> &rivals) {
  cout << "Election started." << endl;
  bool err;
  auto con = Connection::Connect(err);
  if (err) {
    cout << "Failed to connect with spread dawmon to perform election." << endl;
    return false;
  }
  for (auto &rival : rivals)
    con.SendMessage(Message(kElectionMessage, SAFE_MESS), rival);
  auto init = steady_clock::now();
  while(duration_cast<milliseconds>(steady_clock::now() - init).count() < kElectionTimeout) {
    if (con.HasMessage()) {
      auto msg = con.GetMessage();
      if (msg.type() == kAnswerMessage) {
        cout << "Election lost." << endl;
        return false;
      }
    }
  }
  cout << "Election won." << endl;
  return true;
}

/**
 * @brief Manages leadership by performing bully election when there are any
 * changes in servers group membership. Ideally should be run on a separate
 * thread.
 * @param priority of the current server.
 * @param[out] informs if this server is the current leader.
 * @param[in] if changed to true, finishes leadership management.
 */
void Leadership(const string &priority, bool &lead, const bool &quit) {
  bool err;
  auto con = Connection::Connect(priority, err, true);
  if (err) {
    cout << "Failed to connect with spread daemon." << endl;
  }
  if (!con.JoinGroup(kLeadershipGroup)) {
    cout << "Failed to join group: " << kLeadershipGroup << endl;
    return;
  }
  // holds the result of the current election
  future<bool> election_result;

  while (!quit) {
    if (election_result.valid() &&
        future_status::ready == election_result.wait_for(seconds::zero())) {
      lead = election_result.get();
      if (lead) {
        con.SendMessage(Message(kCoordinatorMessage, SAFE_MESS), kLeadershipGroup);
      }
    }

    if (!con.HasMessage()) continue;
    auto msg = con.GetMessage();
    if (msg.IsMembership()) {
      // Rivals are the candidates with priority greater than the current proc
      vector<string> rivals;
      auto group = msg.group();
      copy_if(begin(group), end(group), back_inserter(rivals),
              [&] (const string &s) {
        cout << "Me: " << con.identifier() << endl;
        cout << "Other: " << s << endl;

        return stoi(con.identifier().substr(1)) < stoi(s.substr(1));});
      election_result = async(launch::async, BullyElection, rivals);
      continue;
    }

    switch (msg.type()) {
    case kElectionMessage:
      con.SendMessage(Message(kAnswerMessage, SAFE_MESS), msg.sender());
      break;
    case kCoordinatorMessage:
      cout << "New coordinator: " << msg.sender() << endl;
      break;
    default:
      cout << "Spurious message received." << endl;
    }
  }
}

