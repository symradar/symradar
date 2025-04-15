export const fastapi = (
  method: string,
  _url: string,
  params: Record<string, any>,
  success_callback: (data: any) => void,
  failure_callback: (error: Error) => void,
  content_type: string = 'application/json',
) => {
  let body = JSON.stringify(params);
  let url = _url;
  let init: RequestInit = {
    method: method,
    headers: {
      'Content-Type': content_type,
    },
  };
  if (method === 'POST') {
    init.body = body;
  } else {
    if (Object.keys(params).length > 0) {
      url += '?' + new URLSearchParams(params).toString();
    }
  }
  console.log('fastapi: fetch ' + url);
  fetch(url, init)
    .then((response: Response) => {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error('Something went wrong');
      }
    })
    .then(success_callback)
    .catch(failure_callback);
};
