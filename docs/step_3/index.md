# Customizing Dashboards and Monitoring

## Grafana

Login to the [Grafana](https://grafana.com/) UI with http://localhost:3000 (replace localhost if you are running the backend services on a different computer) using the credentials specified in the [docker-compose](../../docker/docker-compose.yml) file. In this case the default values are iot_user/change_me. Once logged in create a new dashboard.

## Temperature Panel

Create a new panel and use the following for the query:

```flux
from(bucket: "data")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["topic"] =~ /sensors\/.*\/temperature/)
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "mean")
```

Go to the Transformation panel and add the following transformations:

* Outer join (Field name: Time)
* Organize Fields (Rename the fields as desired)

Finally set the Unit to Celsius.

## Humidity Panel

Create a new panel and use the following for the query:

```flux
from(bucket: "data")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["topic"] =~ /sensors\/.*\/humidity/)
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "mean")
```

Go to the Transformation panel and add the following transformations:

* Outer join (Field name: Time)
* Organize Fields (Rename the fields as desired)

Finally set the Unit to Humidity (%H).

## Pressure Panel

Create a new panel and use the following for the query:

```flux
from(bucket: "data")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["topic"] =~ /sensors\/.*\/pressure/)
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "mean")
```

Go to the Transformation panel and add the following transformations:

* Outer join (Field name: Time)
* Organize Fields (Rename the fields as desired)

Finally set the Unit to Pascals.

## Completed Dashboard

After adding the panels you should end up with something like this:

![Dashboard](/docs/images/basic_dashboard.png)

If you have trouble creating this dashboard you can import it from [basic_dashboard.json](../../docker/basic_dashboard.json), although you will have to change the values in the Organize Fields transformation.

## Next Steps

In terms of going beyond creating simple dashboards there are a number of alerting options configurable with both [InfluxDB](https://www.influxdata.com/products/influxdb/) and [Grafana](https://grafana.com/) with the respective documentation being a good resource to extend the basic system.
