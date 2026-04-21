import boto3

REGION = "us-west-2"
USER_POOL_ID = "us-west-2_4clmQZ3ZT"
NUM_USERS = 100

client = boto3.client("cognito-idp", region_name=REGION)


def create_users():
    for i in range(1, NUM_USERS + 1):
        username = f"TestUser{i}"
        email = f"testuser{i}@example.com"

        try:
            client.admin_create_user(UserPoolId=USER_POOL_ID,
                                     Username=username,
                                     TemporaryPassword="Temp1234!",
                                     UserAttributes=[{
                                         "Name": "email",
                                         "Value": email
                                     }, {
                                         "Name": "email_verified",
                                         "Value": "true"
                                     }],
                                     MessageAction="SUPPRESS")

            print(f"✅ Created: {username}")

        except client.exceptions.UsernameExistsException:
            print(f"⚠️ Skipped (exists): {username}")

        except Exception as e:
            print(f"❌ Error creating {username}: {str(e)}")


if __name__ == "__main__":
    create_users()
