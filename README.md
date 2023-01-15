# Custom_Proxy_Server in C
## Introduction
- C로 작성된 Proxy_Server입니다.
- Proxy Server에 대한 내용과 세부설계과정은 해당 링크에서 확인할 수 있습니다.
- [Proxy_Server(notion)](https://abalone-fahrenheit-80e.notion.site/Custom_Proxy_Server-in-C-7087d48c6039476ca45dbcde600f48e2)
---
## How to run?
- Ubuntu 16.04 환경에서 제작되었습니다.
- Ubuntu내 FireFox의 환경설정이 필요합니다.
- Makefile을 통해 실행파일을 생성할 수 있습니다.
- `./proxy_cache`을 실행하여 proxy server을 동작시킵니다.
- http request를 보내 proxy server에 대한 동작을 확인합니다.
---
## Example
![실행 예시](/image/1.PNG)
- 실행파일을 실행한 후 FireFox browser에서 http주소를 실행한다.

![cache파일 예시](/image/2.PNG)
- 필요한 파일의 갯수만큼 cache파일을 생성한다.

![log파일 예시](/image/3.PNG)
- http주소 접속 시 생긴 log를 기록한다.

![결과물 예시](/image/4.PNG)
- http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file4.html
- 위 링크에 접속 시 결과창이다.

![hostname접근 시 load되는 파일들](/image/5.PNG)
- 링크에 접속 시 로드되는 파일들이다.

![결과물 예시(Hit포함)](/image/6.PNG)
- Hit시 Proxy Server를 통해 저장된 cache파일들을 load하는 것을 확인할 수 있다.

---
## Questions
- Issue탭 혹은 chopmojji@gmail.com으로 문의주시면 감사하겠습니다.
