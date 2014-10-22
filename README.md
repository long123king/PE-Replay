PE-Replay
=========

# Description #
This is a pintool that can analyze target dynamically and output code blocks and "key frames".
It is designed to help analyze target program by both static information(i.e. code blocks) and dynamic information(i.e. registers and stack slots).


# Output #
It will output 2 csv files:
1. frames_001.clg
2. blocks_001.clg


These files can be easily processed by Microsoft Excel or by R language. 
Other project [**timeline**](https://github.com/long123king/timeline) is a front end based on django frameworks, it can help view these csv files.

