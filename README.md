# IMU choice

## MPU6050
* [Datasheet](file:///Users/rbaron/Downloads/MPU-6050.pdf)
  * 5 uA in idle
  * How about in motion detection mode?
  * Power supply 2.3 V - 3.4 V
    * See [AAA discharge curve](https://electronics.stackexchange.com/a/520951)
    * [CR123a discharge curve](https://www.powerstream.com/cr123a-tests.htm)
* [JLCPCB](https://jlcpcb.com/partdetail/TdkInvensense-MPU6050/C24112)
  * Basic part
  * But more expensive: $4
* I already know how to use
* Old, tried and true, many libs to study
* I already have exp with it

## LSM6DSRTR
* [Datasheet](file:///Users/rbaron/Downloads/LSM6DSRTR.pdf)
  * 3 uA @ idle
  * How about in "motion detection" mode?
  * Power supply 1.7 V - 3.6 V
* [JLCPCB](https://jlcpcb.com/partdetail/Stmicroelectronics-LSM6DSRTR/C784817)
  * $1.18
  * Extended part
* [Current gen](https://community.st.com/t5/mems-sensors/please-help-me-with-the-difference-between-lsm6dsowtr-and/m-p/193645/highlight/true#M6299)

## LSM6DSLTR
* [Datasheet](file:///Users/rbaron/Downloads/LSM6DSLTR.pdf)
  * Similar to the one above, but only does up to 2000 dps (instead of 4k dps)
    * Makes no difference for us
  * Took over from [LSM6DS3](https://community.st.com/t5/mems-sensors/compatibility-between-lsm6dsl-vs-lsm6ds3/td-p/163949)
