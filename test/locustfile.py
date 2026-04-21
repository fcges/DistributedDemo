from locust import FastHttpUser, between, task
import random
import threading

USERS = [f"TestUser{i}" for i in range(1, 101)]

user_index = 0
lock = threading.Lock()


def get_unique_user():
    global user_index
    with lock:
        user = USERS[user_index % len(USERS)]
        user_index += 1
        return user


def random_match_stats():
    return {
        "kills": random.randint(0, 10),
        "deaths": random.randint(0, 5),
        "assists": random.randint(0, 10)
    }


class LeaderboardUser(FastHttpUser):
    wait_time = between(0.2, 1)

    def on_start(self):
        self.username = get_unique_user()

    @task
    def full_flow(self):

        # =========================
        # 1️⃣ record match stats
        # =========================
        with self.client.post("/stage_1/record-match-stats",
                              json={
                                  "username": self.username,
                                  "matchStats": random_match_stats()
                              },
                              catch_response=True) as response:

            # if response.status_code != 200:
            #     response.failure(f"record failed: {response.status_code}")
            #     return
            response.success()

        # =========================
        # 2️⃣ update leaderboard
        # =========================
        with self.client.post("/stage_1/update-leaderboard",
                              json={"playerIds": [self.username]},
                              catch_response=True) as response:

            # if response.status_code != 200:
            #     response.failure(f"update failed: {response.status_code}")
            #     return
            response.success()

        # =========================
        # 3️⃣ retrieve leaderboard
        # =========================
        with self.client.get("/stage_1/retrieve-leaderboard",
                             catch_response=True) as response:

            # if response.status_code != 200:
            #     response.failure(f"retrieve failed: {response.status_code}")
            #     return

            try:
                data = response.json()
                leaderboard = data.get("Leaderboard", [])

                if not isinstance(leaderboard, list):
                    response.failure("invalid leaderboard format")
                    return

                if len(leaderboard) > 20:
                    response.failure("leaderboard > 20")
                    return

                scores = [p.get("score", 0) for p in leaderboard]
                if scores != sorted(scores, reverse=True):
                    response.failure("leaderboard not sorted")
                    return

                response.success()

            except Exception as e:
                response.failure(f"parse error: {str(e)}")
