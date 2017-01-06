# esp8266-lock

## API

### Get status from server

GET

	/status
  
HEADERS

	apikey 

Response

	{
		"timestamp":1481043718,
		"db_update":312564659,
		"open":312594656,
		"lock":true
	}

======================================================

### Get users from server

GET

	/users/?offset=50
  
HEADERS

	apikey 

Response

	{
		"users": [
			{
				"mac":"90-E6-BA-EE-40-B1",
				"key":"8TYF3VmStnMy"
			},
			{
				"mac":"00-26-37-BD-39-42",
				"key":"yTstbJQlQXFu"
			},
			{
				"mac":"0A-00-27-00-00-14",
				"key":"4xLA0donWu2z"
			},
			{
				"mac":"60-6C-66-24-5C-CB",
				"key":"PScg7HYQntC7"
			},
			{
				"mac":"60-6C-66-24-5C-CA",
				"key":"BhYQuryrUfze"
			},
			{
				"mac":"60-6C-66-24-5C-CE",
				"key":"lDX6BsMg5U6u"
			},
			{
				"mac":"777 94-DB-C9-0E-0A-FC",
				"key":"123456789012"
			},
			{
				"mac":"",
				"key":"hellohello10"
			},
			{
				"mac":"",
				"key":"hellohello11"
			},
			{
				"mac":"",
				"key":"hellohello12"
			}
		],
		"length":10,
		"offset":50,
		"total":72
	}

======================================================

### Save log to server

POST 

	/access-log
  
HEADERS

	apikey

BODY

	{
		"queue": [
			{
				"mac":"94DBC90E0AFC",
				"key":"hellohello12",
				"timestamp":1481043717
			},
			{
				"mac":"94E0AFCDBC90",
				"key":"somekey41234",
				"timestamp":1484563757
			}
		]
	}


Response

	empty


  
